#!/usr/bin/env python3
# Fails unless tag (e.g. v0.4.9) matches idf_component.yml version
import re, sys

tag = sys.argv[1].removeprefix("v")
version = re.search(r'^version: "(.+)"', open("idf_component.yml").read(), re.M)[1]
print(f"manifest={version} tag={tag}")
sys.exit(version != tag)
