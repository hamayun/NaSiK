#!/bin/bash

grep "NetCost:" $1 | cut -d' ' -f2 >& $1.extracted
