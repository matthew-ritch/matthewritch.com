#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Aug 27 01:26:15 2023

@author: mritch
"""
import sys

def txt2html(file):
    f = open(file, 'r').read()
    f2 = f.replace('\n', '<br>\n')
    file2 = file.replace('.txt', '.html')
    open(file2, 'w').write(f2)

if __name__ == '__main__':
    txt2html(sys.argv[1])
    