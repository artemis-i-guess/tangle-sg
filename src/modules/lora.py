# lora_module.py
from sx126x import sx126x
import time

# Setup once globally
lora = sx126x("/dev/tty50", 443, 80, 22, True)


# used configs - 
# addr (address of nodes, must be same for all nodes ) = 80 (16bit address)
sender_addr = 80
receiver_addr = 80
power = 22
freq = 433
receiver_freq = 433
base_freq = 410
offset_freq = 13
# base + offset = actual frequency at which data will be sent 
"""
Data format - 

"""

def send_message(msg: str) -> bool:
    """
    Sends the given string message over LoRa.
    Returns True on success.
    """
    try:
        print(f"[Python] Sending via LoRa...")

        data = bytes([receiver_addr>>8]) + bytes([receiver_addr&0xff]) + bytes([offset_freq]) + bytes([lora.addr>>8]) + bytes([lora.addr&0xff]) + bytes([lora.offset_freq]) + msg.encode()
        
        lora.send(data)

        # the below print block is basically to just dynamically update the terminal output
        print('\x1b[1A',end='\r')
        print(" "*200)
        print('\x1b[1A',end='\r')
        print("[Python] Data sent via LoRa")
        time.sleep(0.2)
        
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
            data = lora.receive()  # Non-blocking or short-blocking read
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