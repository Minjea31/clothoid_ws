#!/usr/bin/env python3
import rospy
import numpy as np
import time
from sensor_msgs.msg import PointCloud2, PointCloud
import sensor_msgs.point_cloud2 as pc2
from visualization_msgs.msg import Marker, MarkerArray
from geometry_msgs.msg import Point32
from sklearn.linear_model import RANSACRegressor
import std_msgs.msg
from scipy.spatial import cKDTree

# === dynamic_reconfigure import ===
from dynamic_reconfigure.server import Server
from detect_ws.cfg import LidarClusteringConfig

# -------------------- 파라미터 영역 (초기값, 동적 파라미터에서 덮어씀) --------------------
PITCH_DEG = 3.3
ROI_X_MIN = 0
ROI_X_MAX = 15
ROI_Y_MIN = -3.0
ROI_Y_MAX = 3.0
ROI_Z_MIN = -2
ROI_Z_MAX = 2
VOXEL_SIZE = 0.1
DROR_MIN_NEIGHBORS = 2
DROR_MIN_RADIUS = 0.1
DROR_RADIUS_SCALE = 0.2
DROR_MAX_RADIUS = 0.2
GROUND_THRESH = 0.25
EUCLIDEAN_MIN_CLUSTER_SIZE = 4
CLUSTER_MERGE_GAP = 0.05
MAX_LENGTH = 2.0
MAX_WIDTH = 2.0
MAX_HEIGHT = 1.5
MIN_LENGTH = 0.1
MIN_WIDTH = 0.1
MIN_HEIGHT = 0.1
TRACKER_MAX_MISS = 3
MATCH_DIST = 1.5
EUCLIDEAN_BASE_DIST = 0.08        # 추가
EUCLIDEAN_DIST_SCALE = 0.012      # 추가
# ------------------------------------------------------------------------------------------

def dyn_reconf_callback(config, level):
    global PITCH_DEG, ROI_X_MIN, ROI_X_MAX, ROI_Y_MIN, ROI_Y_MAX, ROI_Z_MIN, ROI_Z_MAX
    global VOXEL_SIZE, DROR_MIN_NEIGHBORS, DROR_MIN_RADIUS, DROR_RADIUS_SCALE, DROR_MAX_RADIUS
    global GROUND_THRESH, EUCLIDEAN_MIN_CLUSTER_SIZE, CLUSTER_MERGE_GAP
    global MAX_LENGTH, MAX_WIDTH, MAX_HEIGHT, MIN_LENGTH, MIN_WIDTH, MIN_HEIGHT
    global TRACKER_MAX_MISS, MATCH_DIST, EUCLIDEAN_BASE_DIST, EUCLIDEAN_DIST_SCALE

    PITCH_DEG = config.PITCH_DEG
    ROI_X_MIN = config.ROI_X_MIN
    ROI_X_MAX = config.ROI_X_MAX
    ROI_Y_MIN = config.ROI_Y_MIN
    ROI_Y_MAX = config.ROI_Y_MAX
    ROI_Z_MIN = config.ROI_Z_MIN
    ROI_Z_MAX = config.ROI_Z_MAX
    VOXEL_SIZE = config.VOXEL_SIZE
    DROR_MIN_NEIGHBORS = config.DROR_MIN_NEIGHBORS
    DROR_MIN_RADIUS = config.DROR_MIN_RADIUS
    DROR_RADIUS_SCALE = config.DROR_RADIUS_SCALE
    DROR_MAX_RADIUS = config.DROR_MAX_RADIUS
    GROUND_THRESH = config.GROUND_THRESH
    EUCLIDEAN_MIN_CLUSTER_SIZE = config.EUCLIDEAN_MIN_CLUSTER_SIZE
    CLUSTER_MERGE_GAP = config.CLUSTER_MERGE_GAP
    MAX_LENGTH = config.MAX_LENGTH
    MAX_WIDTH = config.MAX_WIDTH
    MAX_HEIGHT = config.MAX_HEIGHT
    MIN_LENGTH = config.MIN_LENGTH
    MIN_WIDTH = config.MIN_WIDTH
    MIN_HEIGHT = config.MIN_HEIGHT
    TRACKER_MAX_MISS = config.TRACKER_MAX_MISS
    MATCH_DIST = config.MATCH_DIST
    EUCLIDEAN_BASE_DIST = config.EUCLIDEAN_BASE_DIST
    EUCLIDEAN_DIST_SCALE = config.EUCLIDEAN_DIST_SCALE
    return config

class KalmanFilter:
    def __init__(self, dt=0.1, process_noise=5.0, measurement_noise=0.1):
        self.dt = dt
        self.A = np.array([[1, 0, dt, 0],
                           [0, 1, 0, dt],
                           [0, 0, 1,  0],
                           [0, 0, 0,  1]])
        self.H = np.array([[1, 0, 0, 0],
                           [0, 1, 0, 0]])
        self.Q = process_noise * np.eye(4)
        self.R = measurement_noise * np.eye(2)
        self.P = np.eye(4)
        self.x = np.zeros((4, 1))

    def predict(self):
        self.x = self.A @ self.x
        self.P = self.A @ self.P @ self.A.T + self.Q
        return self.x[:2].flatten()

    def update(self, z):
        z = np.reshape(z, (2, 1))
        y = z - self.H @ self.x
        S = self.H @ self.P @ self.H.T + self.R
        K = self.P @ self.H.T @ np.linalg.inv(S)
        self.x += K @ y
        self.P = (np.eye(4) - K @ self.H) @ self.P

class Tracker:
    def __init__(self, centroid, tracker_id, dt=0.1):
        self.id = tracker_id
        self.kf = KalmanFilter(dt=dt)
        self.kf.update(centroid)
        self.miss_count = 0
        self.last_pos = centroid

    def predict(self):
        pos = self.kf.predict()
        self.last_pos = pos
        return pos

    def update(self, centroid):
        self.kf.update(centroid)
        self.miss_count = 0
        self.last_pos = centroid

    def miss(self):
        self.kf.predict()
        self.miss_count += 1

def pitch_correction(points, pitch_deg=PITCH_DEG):
    theta = np.deg2rad(pitch_deg)
    R = np.array([
        [ np.cos(theta), 0, np.sin(theta)],
        [ 0,             1, 0            ],
        [-np.sin(theta), 0, np.cos(theta)]
    ])
    return points @ R.T

ROI = {
    "x": (ROI_X_MIN, ROI_X_MAX),
    "y": (ROI_Y_MIN, ROI_Y_MAX),
    "z": (ROI_Z_MIN, ROI_Z_MAX)
}

last_callback_time = [None]
trackers = {}
tracker_id_seq = [0]
prev_tracker_ids = set()

def voxel_downsample(points, voxel_size=VOXEL_SIZE):
    discrete = np.floor(points / voxel_size)
    _, idx = np.unique(discrete, axis=0, return_index=True)
    return points[idx]

def dror(points, min_neighbors=DROR_MIN_NEIGHBORS, min_radius=DROR_MIN_RADIUS, radius_scale=DROR_RADIUS_SCALE, max_radius=DROR_MAX_RADIUS):
    if len(points) == 0:
        return points
    xy = points[:, :2]
    ranges = np.linalg.norm(xy, axis=1)
    radii = np.clip(min_radius + radius_scale * ranges, min_radius, max_radius)
    tree = cKDTree(points)
    neighbor_counts = np.array([len(tree.query_ball_point(points[i], radii[i])) - 1 for i in range(len(points))])
    mask = neighbor_counts >= min_neighbors
    return points[mask]

def remove_ground_ransac(points, threshold=GROUND_THRESH):
    X = points[:, :2]
    y = points[:, 2]
    ransac = RANSACRegressor(residual_threshold=threshold)
    try:
        ransac.fit(X, y)
        z_pred = ransac.predict(X)
        residuals = np.abs(y - z_pred)
        mask = residuals > threshold
        return points[mask]
    except:
        return points

def filter_by_bounding_box(cluster_points, max_length=MAX_LENGTH, max_width=MAX_WIDTH, max_height=MAX_HEIGHT, min_length=MIN_LENGTH, min_width=MIN_WIDTH, min_height=MIN_HEIGHT):
    x_len = np.ptp(cluster_points[:, 0])
    y_len = np.ptp(cluster_points[:, 1])
    z_len = np.ptp(cluster_points[:, 2])
    return (min_length < x_len < max_length) and (min_width < y_len < max_width) and (min_height < z_len < max_height)

def euclidean_clustering_distance_weighted(points, min_cluster_size=EUCLIDEAN_MIN_CLUSTER_SIZE, base_dist=EUCLIDEAN_BASE_DIST, dist_scale=EUCLIDEAN_DIST_SCALE):
    if len(points) == 0:
        return np.array([], dtype=int)
    tree = cKDTree(points[:, :2])
    n_points = len(points)
    visited = np.zeros(n_points, dtype=bool)
    labels = -np.ones(n_points, dtype=int)
    cluster_id = 0

    for i in range(n_points):
        if visited[i]:
            continue
        # x좌표(전방 거리)에 따라 threshold 조정
        dist_th = base_dist + dist_scale * abs(points[i, 0])
        idxs = tree.query_ball_point(points[i, :2], dist_th)
        if len(idxs) < min_cluster_size:
            visited[i] = True
            continue
        queue = list(idxs)
        labels[queue] = cluster_id
        visited[queue] = True
        while queue:
            curr = queue.pop()
            curr_dist = base_dist + dist_scale * abs(points[curr, 0])
            neighbors = tree.query_ball_point(points[curr, :2], curr_dist)
            for n in neighbors:
                if not visited[n]:
                    visited[n] = True
                    if len(tree.query_ball_point(points[n, :2], curr_dist)) >= min_cluster_size:
                        queue.append(n)
                    labels[n] = cluster_id
        cluster_id += 1
    return labels

def merge_close_clusters(points, labels, max_gap=CLUSTER_MERGE_GAP):
    unique_labels = set(labels)
    unique_labels.discard(-1)
    centroids = {l: np.mean(points[labels == l], axis=0) for l in unique_labels}
    merged_labels = dict()
    merged = set()
    for l1 in unique_labels:
        if l1 in merged:
            continue
        merged_labels[l1] = l1
        for l2 in unique_labels:
            if l1 == l2 or l2 in merged:
                continue
            dist = np.linalg.norm(centroids[l1][:2] - centroids[l2][:2])
            if dist < max_gap:
                merged_labels[l2] = l1
                merged.add(l2)
    new_labels = np.array([merged_labels.get(lbl, -1) if lbl != -1 else -1 for lbl in labels])
    return new_labels

def compute_obb_marker(cluster_points, cluster_id):
    points_2d = cluster_points[:, :2]
    centroid = np.mean(points_2d, axis=0)
    centered = points_2d - centroid
    cov = np.cov(centered.T)
    eig_vals, eig_vecs = np.linalg.eig(cov)
    order = np.argsort(eig_vals)[::-1]
    eig_vecs = eig_vecs[:, order]
    rotated = centered @ eig_vecs
    min_xy = np.min(rotated, axis=0)
    max_xy = np.max(rotated, axis=0)
    size = max_xy - min_xy
    center_local = (max_xy + min_xy) / 2.0
    obb_center = centroid + eig_vecs @ center_local
    yaw = np.arctan2(eig_vecs[1, 0], eig_vecs[0, 0])
    qz = np.sin(yaw / 2.0)
    qw = np.cos(yaw / 2.0)
    marker = Marker()
    marker.header.frame_id = "livox_frame"
    marker.header.stamp = rospy.Time.now()
    marker.ns = "obb_boxes"
    marker.id = cluster_id
    marker.type = Marker.CUBE
    marker.action = Marker.ADD
    marker.pose.position.x = obb_center[0]
    marker.pose.position.y = obb_center[1]
    marker.pose.position.z = np.mean(cluster_points[:, 2])
    marker.pose.orientation.z = qz
    marker.pose.orientation.w = qw
    marker.scale.x = size[0]
    marker.scale.y = size[1]
    marker.scale.z = 1.0
    marker.color.r = 1.0
    marker.color.g = 1.0
    marker.color.b = 0.0
    marker.color.a = 0.6
    return marker

def publish_downsampled_points(points):
    header = std_msgs.msg.Header()
    header.stamp = rospy.Time.now()
    header.frame_id = "livox_frame"
    cloud_msg = pc2.create_cloud_xyz32(header, points)
    downsample_pub.publish(cloud_msg)

def publish_preprocessed_points(points):
    header = std_msgs.msg.Header()
    header.stamp = rospy.Time.now()
    header.frame_id = "livox_frame"
    cloud_msg = pc2.create_cloud_xyz32(header, points)
    preprocessed_pub.publish(cloud_msg)

def match_and_update_trackers(observed_centroids):
    global trackers, tracker_id_seq
    assigned_trackers = set()
    assigned_observations = set()
    obs = np.array(observed_centroids)   # (N, 2)
    preds = np.array([trk.predict() for trk in trackers.values()]) if trackers else np.zeros((0,2))
    tracker_keys = list(trackers.keys())

    # greedy matching: 가장 가까운 관측값-트래커만 매칭
    if len(obs) > 0 and len(preds) > 0:
        dists = np.linalg.norm(obs[:, None, :] - preds[None, :, :], axis=2)
        while True:
            min_idx = np.unravel_index(np.argmin(dists), dists.shape)
            i, j = min_idx
            if dists[i, j] > MATCH_DIST:
                break
            trackers[tracker_keys[j]].update(obs[i])
            assigned_trackers.add(tracker_keys[j])
            assigned_observations.add(i)
            dists[i, :] = np.inf
            dists[:, j] = np.inf
            if np.isinf(dists).all():
                break

    # 관측되지 않은 트래커는 예측만 수행 (miss 카운트 증가)
    for k, trk in trackers.items():
        if k not in assigned_trackers:
            trk.miss()
    # miss_count 초과한 트래커는 삭제
    trackers_to_remove = [k for k, t in trackers.items() if t.miss_count > TRACKER_MAX_MISS]
    for k in trackers_to_remove:
        del trackers[k]

    # 남은 관측값은 새 트래커로 등록
    for i, centroid in enumerate(observed_centroids):
        if i not in assigned_observations:
            trackers[tracker_id_seq[0]] = Tracker(centroid, tracker_id_seq[0])
            tracker_id_seq[0] += 1

def pointcloud_callback(msg):
    global last_callback_time, prev_tracker_ids

    now = time.time()
    if last_callback_time[0] is not None:
        print(f"[콜백 간격] {now - last_callback_time[0]:.3f}초")
    last_callback_time[0] = now

    t_total_start = time.time()

    points = np.array([[p[0], p[1], p[2]] for p in pc2.read_points(
        msg, field_names=("x", "y", "z"), skip_nans=True)])
    if len(points) == 0:
        print("입력 포인트 없음")
        return

    # pitch 보정
    points = pitch_correction(points, pitch_deg=PITCH_DEG)

    x_cond = (ROI["x"][0] <= points[:, 0]) & (points[:, 0] <= ROI["x"][1])
    y_cond = (ROI["y"][0] <= points[:, 1]) & (points[:, 1] <= ROI["y"][1])
    z_cond = (ROI["z"][0] <= points[:, 2]) & (points[:, 2] <= ROI["z"][1])
    roi_points = points[x_cond & y_cond & z_cond]
    if len(roi_points) == 0:
        print("ROI 통과 포인트 없음")
        return

    # 다운샘플 publish
    downsampled_points = voxel_downsample(roi_points, voxel_size=VOXEL_SIZE)
    if len(downsampled_points) == 0:
        print("다운샘플 포인트 없음")
        return
    publish_downsampled_points(downsampled_points)

    dror_points = dror(downsampled_points,
                       min_neighbors=DROR_MIN_NEIGHBORS,
                       min_radius=DROR_MIN_RADIUS,
                       radius_scale=DROR_RADIUS_SCALE,
                       max_radius=DROR_MAX_RADIUS)
    if len(dror_points) == 0:
        print("DROR 후 포인트 없음")
        return

    non_ground_points = remove_ground_ransac(dror_points, threshold=GROUND_THRESH)
    if len(non_ground_points) == 0:
        print("지면 제거 후 포인트 없음")
        return

    # === 전처리 완료 포인트 publish ===
    publish_preprocessed_points(non_ground_points)

    # 거리 가중 Euclidean clustering 적용
    labels = euclidean_clustering_distance_weighted(
        non_ground_points,
        min_cluster_size=EUCLIDEAN_MIN_CLUSTER_SIZE,
        base_dist=EUCLIDEAN_BASE_DIST,
        dist_scale=EUCLIDEAN_DIST_SCALE
    )

    # 클러스터 병합 (merge_close_clusters)
    labels = merge_close_clusters(non_ground_points, labels, max_gap=CLUSTER_MERGE_GAP)

    # 관측 centroid 계산
    observed_centroids = []
    clusters_for_marker = {}

    for cluster_id in set(labels):
        if cluster_id == -1:
            continue
        cluster_points = non_ground_points[labels == cluster_id]
        if len(cluster_points) == 0:
            continue
        if not filter_by_bounding_box(cluster_points):
            continue
        centroid = np.mean(cluster_points[:, :2], axis=0)
        observed_centroids.append(centroid)
        clusters_for_marker[cluster_id] = cluster_points

    # 트래커-관측값 매칭 및 예측/업데이트
    match_and_update_trackers(observed_centroids)

    # Publish
    marker_array = MarkerArray()
    cluster_pos_markers = MarkerArray()
    centroid_points = []

    curr_tracker_ids = set()
    for trk in trackers.values():
        centroid = trk.last_pos
        centroid_points.append(Point32(centroid[0], centroid[1], 0.0))
        curr_tracker_ids.add(trk.id)

        # centroid marker (관측/예측 색상 구분)
        pos_marker = Marker()
        pos_marker.header.frame_id = "livox_frame"
        pos_marker.header.stamp = rospy.Time.now()
        pos_marker.ns = "cluster_positions"
        pos_marker.id = trk.id
        pos_marker.type = Marker.SPHERE
        pos_marker.action = Marker.ADD
        pos_marker.pose.position.x = centroid[0]
        pos_marker.pose.position.y = centroid[1]
        pos_marker.pose.position.z = 0.0
        pos_marker.scale.x = 0.3
        pos_marker.scale.y = 0.3
        pos_marker.scale.z = 0.3
        if trk.miss_count == 0:
            pos_marker.color.r = 1.0
            pos_marker.color.g = 0.0
            pos_marker.color.b = 0.0
            pos_marker.color.a = 0.8
        else:
            pos_marker.color.r = 0.0
            pos_marker.color.g = 0.0
            pos_marker.color.b = 1.0
            pos_marker.color.a = 0.4
        cluster_pos_markers.markers.append(pos_marker)

        # OBB bounding box marker publish
        min_dist, best_id = 1e9, None
        for cid, pts in clusters_for_marker.items():
            c = np.mean(pts[:, :2], axis=0)
            d = np.linalg.norm(c - centroid)
            if d < min_dist:
                min_dist = d
                best_id = cid
        if best_id is not None:
            marker = compute_obb_marker(clusters_for_marker[best_id], trk.id)
            marker_array.markers.append(marker)

    # 삭제 마커 처리
    removed_ids = prev_tracker_ids - curr_tracker_ids
    for rem_id in removed_ids:
        del_marker = Marker()
        del_marker.header.frame_id = "livox_frame"
        del_marker.header.stamp = rospy.Time.now()
        del_marker.ns = "obb_boxes"
        del_marker.id = rem_id
        del_marker.action = Marker.DELETE
        marker_array.markers.append(del_marker)

        del_pos_marker = Marker()
        del_pos_marker.header.frame_id = "livox_frame"
        del_pos_marker.header.stamp = rospy.Time.now()
        del_pos_marker.ns = "cluster_positions"
        del_pos_marker.id = rem_id
        del_pos_marker.action = Marker.DELETE
        cluster_pos_markers.markers.append(del_pos_marker)

    prev_tracker_ids = curr_tracker_ids

    marker_pub.publish(marker_array)
    cluster_pos_pub.publish(cluster_pos_markers)

    pc_msg = PointCloud()
    pc_msg.header.stamp = rospy.Time.now()
    pc_msg.header.frame_id = "livox_frame"
    pc_msg.points = centroid_points
    centroid_pub.publish(pc_msg)

    t_total_end = time.time()
    print(f"[콜백 전체 처리시간] {t_total_end - t_total_start:.3f}초\n")

if __name__ == "__main__":
    rospy.init_node("euclidean_clustering")

    # === dynamic_reconfigure 서버 생성 ===
    srv = Server(LidarClusteringConfig, dyn_reconf_callback)

    rospy.Subscriber("/livox/lidar", PointCloud2, pointcloud_callback)
    marker_pub = rospy.Publisher("/lidar_clusters", MarkerArray, queue_size=1)
    cluster_pos_pub = rospy.Publisher("/cluster_positions", MarkerArray, queue_size=10)
    downsample_pub = rospy.Publisher("/downsampled_points", PointCloud2, queue_size=1)
    centroid_pub = rospy.Publisher("/cluster_centroids", PointCloud, queue_size=1)
    preprocessed_pub = rospy.Publisher("/preprocessed_points", PointCloud2, queue_size=1)

    rospy.spin()
