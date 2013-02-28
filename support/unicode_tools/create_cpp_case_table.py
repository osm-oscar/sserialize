#!/usr/bin/python

import os

caseTableFileName = os.path.normpath(os.path.join(os.getcwd(), 'UnicodeData.txt'))
caseTableOutFileName = os.path.normpath(os.path.join(os.getcwd(), 'unicode_case_table.cpp'))

caseTableFile = open(caseTableFileName,  'r')

casemap = []

for line in caseTableFile:
	spl = line.rsplit(';')
	caseinfo = [spl[0], spl[12], spl[13]]
	if ((len(spl[12]) > 0 or len(spl[13]) > 0) and (len(spl[0]) < 5) and (len(spl[12]) < 5) and (len(spl[13]) < 5)):
		casemap.append(caseinfo)

##Print our map

print '#define UNICODE_CASE_TABLE_SIZE 2000'

print 'uint16_t unicode_source_case_table[] = {'
for cases in casemap:
	print '0x' + cases[0] + ','
print '};'
	
print 'uint16_t unicode_upper_case_table[] = {'
for cases in casemap:
	if (len(cases[1]) > 0):
		print '0x' + cases[1] + ','
	else:
		print '0x0,' 
print '};'

print 'uint16_t unicode_lower_case_table[] = {'
for cases in casemap:
	if (len(cases[2]) > 0):
		print '0x' + cases[2] + ','
	else:
		print '0x0,'
print '};'

caseTableFile.close()
