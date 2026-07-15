import socket

class ZeroTrustClient:
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
        self.shift = 3  # Caesar cipher shift 

    def _encrypt(self, text):
        """Encrypts data BEFORE it reaches the database"""
        encrypted = ""
        for char in text:
            if char.isalpha():
                offset = 65 if char.isupper() else 97
                encrypted += chr((ord(char) - offset + self.shift) % 26 + offset)
            else:
                encrypted += char
        return encrypted

    def _decrypt(self, text):
        decrypted = ""
        for char in text:
            if char.isalpha():
                offset = 65 if char.isupper() else 97
                decrypted += chr((ord(char) - offset - self.shift) % 26 + offset)
            else:
                decrypted += char
        return decrypted

    def _send_command(self, command_string):
        """Opens a TCP socket, sends command, receives response."""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((self.host, self.port))
                s.sendall(command_string.encode('utf-8'))
                data = s.recv(1024)
                return data.decode('utf-8')
        except ConnectionRefusedError:
            return "ERROR_CONNECTION_REFUSED: Is C Server running?"

    def add_password(self, service, password):
        encrypted_pass = self._encrypt(password)
        return self._send_command(f"ADD {service} {encrypted_pass}")

    def get_password(self, service):
        response = self._send_command(f"GET {service}")
        if response.startswith("FOUND "):
            encrypted_pass = response.split(" ")[1]
            return self._decrypt(encrypted_pass)
        return response

    def delete_password(self, service):
        return self._send_command(f"DEL {service}")
