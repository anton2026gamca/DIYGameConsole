#!/bin/bash
echo $((`i2cget -y 1 0x57 0x02` >> 7))
