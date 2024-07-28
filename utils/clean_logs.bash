#!/bin/bash
find .. -type f -name "*.log" -exec truncate -s 0 {} \;