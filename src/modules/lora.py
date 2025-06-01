# lora_module.py
from sx126x import SX126x
import time

# Setup once globally
lora = SX126x()
lora.begin()

def send_message(msg: str) -> bool:
    """
    Sends the given string message over LoRa.
    Returns True on success.
    """
    try:
        print(f"[Python] Sending via LoRa: {msg}")
        lora.send(msg.encode())
        return True
    except Exception as e:
        print(f"[Python] send_message error: {e}")
        return False
        
def receive_message(timeout: float = 2.0) -> str:
    """
    Blocks (up to `timeout` seconds) waiting for a LoRa packet.
    Returns the received message as a UTF-8 string, or an empty string on timeout.
    """
    try:
        start = time.time()
        while True:
            data = lora.recv()  # Non-blocking or short-blocking read
            if data:
                try:
                    text = data.decode()
                    print(f"[Python] Received via LoRa: {text}")
                    return text
                except UnicodeDecodeError:
                    print("[Python] Warning: received non-UTF8 data.")
                    return ""
            if time.time() - start > timeout:
                return ""
            time.sleep(0.05)
    except Exception as e:
        print(f"[Python] receive_message error: {e}")
        return ""