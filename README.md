# File-System-Dump-and-Analysis
Project 3 for my CS 111 class

For part A of this project (dump), I mounted an image file on my own linux machine and investigated it with debugfs(8). I had to write a program that analyzed the image file and outputed a summary to six csv files describing the super block, cylinder groups, free-lists, i-nodes, indirect blocks, and directories. 

For part B (analysis), I read in the six csv files produced from part A and checked them for certain file system corruptions. Then these errors were reported to a file called "lab3b_check.txt" which was created and put on the current directory when the part B analysis was run. 
