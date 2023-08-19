# 19-08-2023 Performance fix

Request-response approach is not effective. Possible alternatives:
- Request multiple DataMessages to be returned
- Move full stream control to server: Server sends data and client keeps sending "More" every X messages
