#!/bin/bash

grep "Time Used :" $1 | cut -d' ' -f4 >& $1.extracted
