import cv2
import socket
from cvzone.HandTrackingModule import HandDetector

cap = cv2.VideoCapture(0)
cap.set(3, 1280)
cap.set(4, 720)

detector = HandDetector(detectionCon=0.7, maxHands=1)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('127.0.0.1', 5005)

prev_x, prev_y = 0, 0
smoothing_factor = 6.0
left_click_active = 0
right_click_active = 0

while True:
    success, img = cap.read()
    if not success:
        continue

    img = cv2.flip(img, 1)
    hands, img = detector.findHands(img, draw=True)

    if hands:
        hand = hands[0]
        lmList = hand["lmList"]
        
        index_x, index_y = lmList[8][0], lmList[8][1]
        middle_x, middle_y = lmList[12][0], lmList[12][1]
        thumb_x, thumb_y = lmList[4][0], lmList[4][1]
        
        h, w, c = img.shape
        raw_norm_x = index_x / w
        raw_norm_y = index_y / h
        
        if prev_x == 0 and prev_y == 0:
            prev_x, prev_y = raw_norm_x, raw_norm_y
        
        smooth_x = prev_x + (raw_norm_x - prev_x) / smoothing_factor
        smooth_y = prev_y + (raw_norm_y - prev_y) / smoothing_factor
        prev_x, prev_y = smooth_x, smooth_y
        
        left_dist, _, img = detector.findDistance((index_x, index_y), (thumb_x, thumb_y), img)
        right_dist, _, img = detector.findDistance((middle_x, middle_y), (thumb_x, thumb_y), img)
        
        if left_dist < 30:
            left_click_active = 1
        elif left_dist > 50:
            left_click_active = 0

        if right_dist < 30:
            right_click_active = 1
        elif right_dist > 50:
            right_click_active = 0
        
        payload = f"{smooth_x:.4f},{smooth_y:.4f},{left_click_active},{right_click_active}"
        sock.sendto(payload.encode('utf-8'), server_address)

    cv2.imshow("Wisdom Park Hand Tracker", img)
    
    if cv2.waitKey(5) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()