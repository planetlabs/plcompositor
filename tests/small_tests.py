import unittest
import sys
import os
import shutil
import numpy
import traceback

from osgeo import gdal, gdal_array

TEMPLATE_GRAY = 'data/2x2_gray_template.tif'
TEMPLATE_RGB  = 'data/2x2_rgb_template.tif'
TEMPLATE_RGBA = 'data/2x2_rgba_template.tif'

class Tests(unittest.TestCase):

    def setUp(self):
        self.temp_test_files = []
        # Ensure we have saojose data unpacked.
        if not os.path.exists('saojose'):
            self.fetch_data('saojose_l8_chip.zip')
            ZipFile('saojose_l8_chip.zip', 'r').extractall()

    def tearDown(self):
        pass

    def make_file(self, template_filename, data=None):
        caller = traceback.extract_stack(limit=2)[0][2]
        new_filename = '%s_%d.tif' % (caller, len(self.temp_test_files))

        shutil.copyfile(template_filename, new_filename)
        self.temp_test_files.append(new_filename)
        
        if data is not None:
            data = numpy.array(data)
            ds = gdal.Open(new_filename, gdal.GA_Update)
            if len(data.shape) == 2:
                ds.GetRasterBand(1).WriteArray(data)
            else:
                assert ds.RasterCount == data.shape[0]
                for bi in range(ds.RasterCount):
                    ds.GetRasterBand(bi+1).WriteArray(data[bi])
            ds = None

        return new_filename

    def clean_files(self):
        for filename in self.temp_test_files:
            os.unlink(filename)
        self.temp_test_files = []

    def compare_file(self, test_file, golden_data):
        test_data = gdal_array.LoadFile(test_file)
        
        if golden_data is None:
            print 'No golden data, %s is:' % test_file
            print test_data.tolist()
            raise Exception('No golden data, %s is:' % test_file)

        if not numpy.array_equal(test_data, numpy.array(golden_data)):
            print '%s differs from golden data:' % test_file
            print
            print test_file
            print test_data.tolist()
            print
            print 'golden data:'
            print golden_data
            
            raise Exception('%s differs from golden data' % test_file)

    def run_compositor(self, args):
        if os.path.exists('../compositor'):
            binary = '../compositor'
        else:
            binary = 'compositor'
        cmd = ' '.join([binary] + args)
        print cmd
        return os.system(cmd)

    def test_small_darkest_gray(self):
        test_file = self.make_file(TEMPLATE_GRAY)
        
        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-o', test_file, 
            '-i', 
            self.make_file(TEMPLATE_GRAY, [[0, 1], [3, 4]]),
            '-i',
            self.make_file(TEMPLATE_GRAY, [[9, 8], [6, 5]]),
            ]

        self.run_compositor(args)

        self.compare_file(test_file, [[0, 1], [3, 4]])

        self.clean_files()
        
    def test_small_darkest_rgb(self):
        test_file = self.make_file(TEMPLATE_RGB)
        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-o', test_file, 
            '-i',
            self.make_file(TEMPLATE_RGB, 
                           [[[0, 1], [3, 4]],
                            [[0, 1], [3, 4]],
                            [[0, 1], [3, 4]]]),
            '-i',
            self.make_file(TEMPLATE_RGB, 
                           [[[9, 8], [6, 5]],
                            [[9, 8], [6, 5]],
                            [[9, 8], [6, 5]]]),
            ]

        self.run_compositor(args)

        self.compare_file(test_file, 
                          [[[0, 1], [3, 4]], 
                           [[0, 1], [3, 4]], 
                           [[0, 1], [3, 4]]])

        self.clean_files()
        
    def test_small_darkest_rgba(self):
        test_file = self.make_file(TEMPLATE_RGBA)
        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-o', test_file, 
            '-i',
            self.make_file(TEMPLATE_RGBA, 
                           [[[0, 1], [3, 4]],
                            [[0, 1], [3, 4]],
                            [[0, 1], [3, 4]],
                            [[255, 255], [255, 255]]]),
            '-i',
            self.make_file(TEMPLATE_RGBA, 
                           [[[9, 8], [6, 5]],
                            [[9, 8], [6, 5]],
                            [[9, 8], [6, 5]],
                            [[255, 255], [255, 255]]]),
            ]

        self.run_compositor(args)

        self.compare_file(test_file, 
                          [[[0, 1], [3, 4]], 
                           [[0, 1], [3, 4]], 
                           [[0, 1], [3, 4]], 
                           [[255, 255], [255, 255]]])

        self.clean_files()
        
if __name__ == '__main__':
    unittest.main()
