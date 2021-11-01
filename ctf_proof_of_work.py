import itertools
import hashlib


def generate_random_str():
    chars = "0123456789abcdefghijklmnopqrstuvwxyz"
    for i in range(4, 10):
        for item in itertools.product(chars, repeat=i):
            yield "".join(item)


def brute_hash(prefix_str, zero_bit_cnt):
    local_iter = generate_random_str()
    zero_digit_cnt = int(zero_bit_cnt / 4)
    half_digit_flag = zero_bit_cnt % 4
    while True:
        n_data = prefix_str + next(local_iter)
        n_d_sha = hashlib.sha256(n_data.encode("utf-8")).hexdigest()
        if n_d_sha[0:zero_digit_cnt] == "0"*zero_digit_cnt:
            if half_digit_flag and int(n_d_sha[zero_digit_cnt],16) & ((1<<half_digit_flag)-1):
                continue
            print(n_data)
            print(n_d_sha)
            break


if __name__ == "__main__":
    brute_hash("flag",26)
