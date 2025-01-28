import glob
import argparse

def main(directory):
	parser = argparse.ArgumentParser()
	parser.add_argument("filepaths", nargs="+")
	args = parser.parse_args()
	for filepath in args.filepaths:
		for path in glob.glob(filepath + ".glsl.*"):
			
	path = "C:/VulkanSDK/1.3.231.1/Bin/glslc.exe"




