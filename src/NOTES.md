# 19-08-2023 Performance fix

Request-response approach is not effective. Possible alternatives:
- Request multiple DataMessages to be returned
- Move full stream control to server: Server sends data and client keeps
  sending "More" every X messages


Ideas:
- Receive dataMessage and handle it in new thread

# 21-08-2023 Handful approach:

Handful approach
- Request new series of data message, send series number to the library server
- In handleDataMessageFn: if series number differs from previous and buffer is
  able to store new message, then request new series of data

Bugs:
- server sends header before every dataMessage, client expects single header

# 24-08-2023 Handful approach:

New idea for change on client level:
- Use circle circle buffer with element with size of X UDP packages
- Request X packages, each with order number
- Return messages with order number (server part)
- Place each message in buffer in correct place
- Push whole buffer
