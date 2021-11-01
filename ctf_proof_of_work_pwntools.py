from pwn import *
import hashlib

def f(x, prefix, zero_bit_cnt):
    return bin(int(hashlib.sha256((prefix + x).encode('utf-8')).hexdigest(), 16))[2:].rjust(256,'0').startswith("0"*zero_bit_cnt)

def brute_sha256(prefix, zero_bit_cnt):
    return util.iters.mbruteforce(lambda x: f(x, prefix, zero_bit_cnt), string.ascii_letters + string.digits, 5, 'fixed')

if __name__ == "__main__":
    brute_sha256("flag", 24)
