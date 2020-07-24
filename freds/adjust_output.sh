#!/bin/bash

# TODO: sanity check file exist or don't

SOURCE_FILE="${1}"
DEST_FILE="${2}"

# Get line count
TOTAL_LINE_COUNT=$(wc -l ${SOURCE_FILE} | awk -F\  '{print $1}')

# TODO: better way for intelligently determining commented-out lines, for now assume first 3
DATA_LINE_COUNT=$((TOTAL_LINE_COUNT-3))

tail -n ${DATA_LINE_COUNT} ${SOURCE_FILE} | sed 's/ /,/g' > ${DEST_FILE}

