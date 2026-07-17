import cv2
import socket
from cvzone.HandTrackingModule import HandDetector

# Initialize the webcam and force a 720p resolution for accurate tracking
cap = cv2.VideoCapture(0)
cap.set(3, 1280)
cap.set(4, 720)

# detectionCon is the confidence threshold. maxHands ensures it only tracks one user.
detector = HandDetector(detectionCon=0.7, maxHands=1)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('127.0.0.1', 5005)

while True:
    success, img = cap.read()
    if not success:
        continue

    # Flip the image so it acts like a mirror
    img = cv2.flip(img, 1)
    
    # CVZone finds the hand and draws the skeletal landmarks automatically
    hands, img = detector.findHands(img, draw=True)

    if hands:
        hand = hands[0]
        lmList = hand["lmList"] # List of all 21 landmark coordinate points
        
        # Extract pixel coordinates for the tip of the index finger (8) and thumb (4)
        index_x, index_y = lmList[8][0], lmList[8][1]
        thumb_x, thumb_y = lmList[4][0], lmList[4][1]
        
        # Normalize the coordinates (0.0 to 1.0) so your C++ engine scales it to 1920x1080 perfectly
        h, w, c = img.shape
        norm_x = index_x / w
        norm_y = index_y / h
        
        # Use CVZone's built-in distance calculator between the two fingers
        length, info, img = detector.findDistance((index_x, index_y), (thumb_x, thumb_y), img)
        
        # If the fingers pinch close together (under 40 pixels), trigger a click state
        is_pinching = 1 if length < 40 else 0
        
        # Fire the payload to the C++ SFML UdpSocket
        payload = f"{norm_x},{norm_y},{is_pinching}"
        sock.sendto(payload.encode('utf-8'), server_address)

    cv2.imshow("Wisdom Park Hand Tracker", img)
    
    # Press 'ESC' on the camera window to safely close the tracker
    if cv2.waitKey(5) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()