"""
Download files from the website, subdirectories/files are defined as follows:
<a href="4%20-%20Developing%20Testing%20Tools/">4 - Developing Testing Tools/</a>
Recursively parse files/directories from a/href, and splice urls
e.g: http://example.com/courses/4%20-%20Developing%20Testing%20Tools/part_1.pdf
"""
import requests
import subprocess
import os
from urllib.parse import unquote
from bs4 import BeautifulSoup
from urllib.parse import urlparse, urljoin

base_url = "http://example.com/courses/"

def get_links(url):
    if not url.endswith("/"):
        return []

    # print("get_links: " + url)
    response = requests.get(url)
    soup = BeautifulSoup(response.text, "html.parser")
    links = []

    for link in soup.find_all("a"):
        href = link.get("href")
        if href and not href.startswith("?") and not href.startswith("/"):
            full_url = urljoin(url, href)
            links.append(full_url)

    tmp_l = []
    for link in links:
        # print("before get_links: " + link)
        tmp_l.extend(get_links(link))
    links.extend(tmp_l)
    return links


def download_file(url, save_path):
    print(f"Downloading: {url} -> {save_path}")
    subprocess.run(["wget", "-c", "-P", save_path, url])


def create_directories_and_download(base_url, file_url):
    print(f"Downloading: {file_url}")

    parsed_base_url = urlparse(base_url)
    parsed_file_url = urlparse(file_url)

    relative_path = parsed_file_url.path[len(parsed_base_url.path):]
    dir_path = unquote(os.path.dirname(relative_path)).replace(" ", "_")
    if dir_path != "" and not os.path.exists(dir_path):
        print("mkdir: " + dir_path)
        os.makedirs(dir_path)

    download_file(file_url, dir_path)


# main
initial_links = get_links(base_url)
all_links = set(initial_links)

for link in sorted(all_links):
    if not link.endswith("/"):
        create_directories_and_download(base_url, link)
