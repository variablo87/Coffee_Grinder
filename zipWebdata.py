#from jsmin import jsmin
import gzip
from os import walk
import os
import shutil

mypath='data'
#get file list
f = []
for (dirpath, dirnames, filenames) in walk(mypath+'_/'):
    f.extend(filenames)
    break

if not os.path.exists(mypath):
    os.makedirs(mypath)


#if file ends with .js and not with min.js minimise the Javascript
#print 'create min js:'
#for item in f:
#    if (item[len(item)-3:]=='.js')and(item[len(item)-6:]!='min.js'):
#        print item
#        f.append(item[:len(item)-3]+'.min.js')
#        with open(mypath+'_/'+item) as f_in, gzip.open(mypath+'_/'+item[:len(item)-3]+'.min.js', 'wb') as f_out:
#            minified = jsmin(f_in.read()) 
#            f_out.write(minified)
        
#if file ends with min.js zip
print('create folder with gz files:')
for item in f:
    if (1): #(item[len(item)-6:]=='min.js')or(item[len(item)-3:]!='.js'):
        print(item) 
        with open(mypath+'_/'+item, 'rb') as f_in, gzip.open(mypath+'/'+item+'.gz', 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)
