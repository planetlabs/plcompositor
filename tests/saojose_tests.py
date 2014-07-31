import unittest
import sys
import os
import shutil
import urllib
import json
from zipfile import ZipFile

from osgeo import gdal

# Blech, I wish this was a proper library module.

sys.path.append('/usr/bin')
import gdalcompare

SAOJOSE_INPUTS = [
    'saojose/saojose_LC82150642013104LGN01_RGB.tif',
    'saojose/saojose_LC82150642013120LGN01_RGB.tif',
    'saojose/saojose_LC82150642013136LGN01_RGB.tif',
    'saojose/saojose_LC82150642013152LGN00_RGB.tif',
    'saojose/saojose_LC82150642013200LGN00_RGB.tif',
    'saojose/saojose_LC82150642013216LGN00_RGB.tif',
    'saojose/saojose_LC82150642013232LGN00_RGB.tif',
    'saojose/saojose_LC82150642013248LGN00_RGB.tif',
    'saojose/saojose_LC82150642013280LGN00_RGB.tif',
    'saojose/saojose_LC82150642014011LGN00_RGB.tif',
    ]

class Tests(unittest.TestCase):

    def setUp(self):
        self.temp_test_files = []
        # Ensure we have saojose data unpacked.
        if not os.path.exists('saojose'):
            self.fetch_data('saojose_l8_chip.zip')
            ZipFile('saojose_l8_chip.zip', 'r').extractall()

    def tearDown(self):
        pass

    def fetch_data(self, filename):
        # We eventually need some sort of md5 or date checking. 
        if os.path.exists(filename):
            return

        url = 'http://download.osgeo.org/plcompositor/' + filename
        print 'Fetch: %s' % url
        urllib.urlretrieve(url, filename)

        if os.path.exists(filename):
            header = open(filename).read(500)
            if '404 Not Found' in header:
                os.unlink(filename)

    def check(self, golden_file, test_file, options = []):
        self.assertTrue(os.path.exists(test_file))

        self.fetch_data(golden_file)

        msg = ''
        if os.path.exists(golden_file):
            diffs = gdalcompare.compare_db(gdal.Open(golden_file),
                                           gdal.Open(test_file),
                                           options = options)
            if diffs == 0:
                return

            msg += '\n'
            msg += 'Compare:\n'
            msg += '  Golden File: %s\n' % golden_file
            msg += '  Test Output: %s\n' % test_file
        else:
            msg += '\n'
            msg += 'No Golden File Exists.\n'
            msg += '  Test Output: %s\n' % test_file

        if not golden_file.startswith('/'):
            msg += '\n'
            msg += 'If result looks ok, update with:\n'
            msg += '  scp %s download.osgeo.org:/osgeo/download/plcompositor/%s' % (
                test_file, golden_file)

        print msg
        
        raise Exception(msg)

    def run_compositor(self, args):
        if os.path.exists('../compositor'):
            binary = '../compositor'
        else:
            binary = 'compositor'
        cmd = ' '.join([binary] + args)
        print cmd
        return os.system(cmd)

    def test_greenest_simple(self):
        test_file = 'greenest_simple_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'greenest',
            '-o', test_file, 
            '-i', 'saojose/saojose_LC82150642013280LGN00_RGB.tif',
            '-i', 'saojose/saojose_LC82150642014011LGN00_RGB.tif',
            ]

        self.run_compositor(args)

        self.check('greenest_simple_golden.tif', test_file)
        os.unlink(test_file)
        
    def test_greenest_many(self):
        test_file = 'greenest_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'greenest',
            '-o', test_file, 
            ]

        for filename in SAOJOSE_INPUTS:
            args += ['-i', filename]

        
        self.run_compositor(args)

        self.check('greenest_golden.tif', test_file)
        os.unlink(test_file)
        
    def test_darkest_with_source(self):
        test_file = 'darkest_test.tif'
        test_source_file = 'darkest_source_test.tif'
        test_quality_file = 'darkest_quality_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-o', test_file, 
            '-st', test_source_file,
            '-qo', test_quality_file,
            ]

        for filename in SAOJOSE_INPUTS:
            args += ['-i', filename]

        self.run_compositor(args)

        self.check('darkest_golden.tif', test_file)
        self.check('darkest_source_golden.tif', test_source_file)
        self.check('darkest_quality_golden.tif', test_quality_file)
        os.unlink(test_file)
        os.unlink(test_source_file)
        os.unlink(test_quality_file)
        
    def test_newest(self):
        test_file = 'newest_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'scene_measure',
            '-s', 'scene_measure', 'acquisition_date',
            '-o', test_file,
            '-i', 'saojose/saojose_LC82150642013216LGN00_RGB.tif',
            '-qm', 'acquisition_date', '1377000216',
            '-i', 'saojose/saojose_LC82150642014011LGN00_RGB.tif',
            '-qm', 'acquisition_date', '1377001011',
            '-i', 'saojose/saojose_LC82150642013104LGN01_RGB.tif',
            '-qm', 'acquisition_date', '1377000104',
            ]

        self.run_compositor(args)

        self.check('newest_golden.tif', test_file)
        os.unlink(test_file)
        
    def test_newest_json(self):
        json_file = 'newest.json'
        test_file = 'newest_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        control = {
            'output_file': test_file,
            'compositors': [
                {
                    'class': 'scene_measure',
                    'scene_measure': 'acquisition_date',
                    },
                ],
            'inputs': [
                {
                    'filename': 'saojose/saojose_LC82150642013216LGN00_RGB.tif',
                    'acquisition_date': 1377000216.0,
                    },
                {
                    'filename': 'saojose/saojose_LC82150642014011LGN00_RGB.tif',
                    'acquisition_date': 1377001011.0,
                    },
                {
                    'filename': 'saojose/saojose_LC82150642013104LGN01_RGB.tif',
                    'acquisition_date': 1377000104.0,
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        args = [ '-q', '-j', json_file ]
        self.run_compositor(args)

        self.check('newest_golden.tif', test_file)
        os.unlink(test_file)
        os.unlink(json_file)
        
    def test_with_l8_cloud_mask(self):
        test_file = 'l8_cloud_mask_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-s', 'cloud_quality', 'landsat8',
            '-o', test_file, 
            ]

        for filename in SAOJOSE_INPUTS:
            args += ['-i', filename, 
                     '-c', filename.replace('_LC8', '_cld_LC8')]
        
        self.run_compositor(args)

        self.check('l8_cloud_mask_golden.tif', test_file)
        os.unlink(test_file)
        
    def test_median_with_cloud(self):
        test_file = 'median_with_cloud_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-s', 'cloud_quality', 'landsat8',
            '-s', 'compositor', 'median',
            '-o', test_file, 
            ]

        for filename in SAOJOSE_INPUTS:
            args += ['-i', filename, 
                     '-c', filename.replace('_LC8', '_cld_LC8')]
        
        self.run_compositor(args)

        self.check('median_with_cloud_golden.tif', test_file)
        os.unlink(test_file)
        
    def test_median_like_darkest(self):
        # A median filter with median_threshold=1.0 should be the same
        # as a simple quality darkest composite. 
        test_file = 'median_like_darkest_test.tif'
        shutil.copyfile('saojose/saojose_l8_chip.tif', test_file)

        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-s', 'compositor', 'median',
            '-s', 'quality_percentile', '100.0',
            '-o', test_file, 
            ]

        for filename in SAOJOSE_INPUTS:
            args += ['-i', filename]
        
        self.run_compositor(args)

        # Why can't we use darkest_golden.tif?  There are 9 pixels different
        # but I don't yet know why.
        self.check('media_like_darkest_golden.tif', test_file)
        os.unlink(test_file)

if __name__ == '__main__':
    unittest.main()
