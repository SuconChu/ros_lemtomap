#!/bin/bash          
#
# Script to clean (remove) doxygen documentation for all packages
#
# Koen Lekkerkerker
# created: 17 Jun 2014 
# updated: 14 Jul 2014
#
# Usage:
# ./clean_doxygen_all.sh
#
# All doc folders in the packages will be deleted
# Open doc/html/index.html to view the documentation

cd "${0%/*}" #make current working directory the folder of this package

for i in $(find . -name 'generate_doxygen.sh') #store an array of all paths to a generate_doxygen.sh, that can be found from the parent folder of the lemtomap metapackage 
  do rm -r ${i%/*}'/doc'
done
