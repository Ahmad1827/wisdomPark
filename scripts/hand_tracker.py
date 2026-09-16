import cv2
import socket
import math
from cvzone.HandTrackingModule import HandDetector

def open_physical_camera():
    for idx in range(5):
        c = cv2.VideoCapture(idx, cv2.CAP_DSHOW)
        if c.isOpened():
            ret, frame = c.read()
            if ret and frame is not None:
                if frame.mean() > 15:
                    return c
            c.release()
    return cv2.VideoCapture(0)

cap = open_physical_camera()
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_AUTOFOCUS, 1)

detector = HandDetector(detectionCon=0.8, maxHands=1)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('127.0.0.1', 5005)

prev_x, prev_y = 0.0, 0.0
left_click_active = 0
right_click_active = 0
zoom_active = 0
had_hand = False

while True:
    success, img = cap.read()
    if not success:
        continue

    img = cv2.flip(img, 1)
    hands, img = detector.findHands(img, draw=True)

    if hands:
        had_hand = True
        hand = hands[0]
        lmList = hand["lmList"]
        h, w, _ = img.shape

        wrist = lmList[0][:2]
        middle_base = lmList[9][:2]
        hand_scale = math.hypot(middle_base[0] - wrist[0], middle_base[1] - wrist[1])
        if hand_scale < 1.0:
            hand_scale = 1.0

        thumb_tip = lmList[4][:2]
        index_tip = lmList[8][:2]
        middle_tip = lmList[12][:2]
        pinky_tip = lmList[20][:2]

        ratio_left = math.hypot(index_tip[0] - thumb_tip[0], index_tip[1] - thumb_tip[1]) / hand_scale
        ratio_right = math.hypot(middle_tip[0] - thumb_tip[0], middle_tip[1] - thumb_tip[1]) / hand_scale
        ratio_zoom = math.hypot(pinky_tip[0] - thumb_tip[0], pinky_tip[1] - thumb_tip[1]) / hand_scale

        if ratio_left < 0.20:
            left_click_active = 1
        elif ratio_left > 0.30:
            left_click_active = 0

        if ratio_right < 0.22:
            right_click_active = 1
        elif ratio_right > 0.32:
            right_click_active = 0

        if ratio_zoom < 0.24:
            zoom_active = 1
        elif ratio_zoom > 0.36:
            zoom_active = 0

        raw_norm_x = (index_tip[0] - w * 0.15) / (w * 0.7)
        raw_norm_y = (index_tip[1] - h * 0.15) / (h * 0.7)
        raw_norm_x = max(0.0, min(1.0, raw_norm_x))
        raw_norm_y = max(0.0, min(1.0, raw_norm_y))

        if prev_x == 0.0 and prev_y == 0.0:
            prev_x, prev_y = raw_norm_x, raw_norm_y

        smoothing = 14.0 if left_click_active else 5.0
        dx = raw_norm_x - prev_x
        dy = raw_norm_y - prev_y

        if left_click_active and math.hypot(dx, dy) < 0.003:
            smooth_x = prev_x
            smooth_y = prev_y
        else:
            smooth_x = prev_x + dx / smoothing
            smooth_y = prev_y + dy / smoothing

        prev_x, prev_y = smooth_x, smooth_y

        payload = f"POS:{smooth_x:.4f},{smooth_y:.4f},{left_click_active},{right_click_active},{zoom_active}"
        sock.sendto(payload.encode('utf-8'), server_address)

    else:
        if had_hand:
            had_hand = False
            left_click_active = 0
            right_click_active = 0
            zoom_active = 0
            prev_x, prev_y = 0.0, 0.0
            sock.sendto(b"POS:LOST,0,0,0,0", server_address)

    preview = cv2.resize(img, (480, 360))
    enc_ok, enc_jpg = cv2.imencode('.jpg', preview, [int(cv2.IMWRITE_JPEG_QUALITY), 85])
    if enc_ok:
        sock.sendto(b"IMG:" + enc_jpg.tobytes(), server_address)

    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()