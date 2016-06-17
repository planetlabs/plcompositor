import unittest
import sys
import os
import shutil
import numpy
import traceback
import json
import subprocess

from osgeo import gdal, gdal_array

TEMPLATE_FLOAT = 'data/2x2_float_template.tif'
TEMPLATE_GRAY = 'data/2x2_gray_template.tif'
TEMPLATE_RGB  = 'data/2x2_rgb_template.tif'
TEMPLATE_RGBA = 'data/2x2_rgba_template.tif'
TEMPLATE_UINT16 = 'data/2x2_gray_uint16_template.tif'

TEMPLATE_GRAY_3X3 = 'data/3x3_gray_template.tif'
TEMPLATE_FLOAT_3X3 = 'data/3x3_float_template.tif'

class Tests(unittest.TestCase):

    def setUp(self):
        self.temp_test_files = []

    def tearDown(self):
        pass

    def make_file(self, template_filename, data=None, filename = None):
        template_filename = os.path.join(os.path.dirname(__file__),
                                         template_filename)
        if filename is None:
            caller = traceback.extract_stack(limit=2)[0][2]
            filename = '%s_%d.tif' % (caller, len(self.temp_test_files))

        shutil.copyfile(template_filename, filename)
        self.temp_test_files.append(filename)
        
        if data is not None:
            data = numpy.array(data)
            ds = gdal.Open(filename, gdal.GA_Update)
            if len(data.shape) == 2:
                ds.GetRasterBand(1).WriteArray(data)
            else:
                assert ds.RasterCount == data.shape[0]
                for bi in range(ds.RasterCount):
                    ds.GetRasterBand(bi+1).WriteArray(data[bi])
            ds = None

        return filename

    def clean_files(self):
        for filename in self.temp_test_files:
            os.unlink(filename)
        self.temp_test_files = []

    def compare_file(self, test_file, golden_data, tolerance=0.0):
        test_data = gdal_array.LoadFile(test_file)
        
        if golden_data is None:
            print 'No golden data, %s is:' % test_file
            print test_data.tolist()
            raise Exception('No golden data, %s is:' % test_file)

        if not numpy.allclose(test_data, numpy.array(golden_data),atol=tolerance):
            print '%s differs from golden data:' % test_file
            print
            print test_file
            print test_data.tolist()
            print
            print 'golden data:'
            print golden_data
            
            raise Exception('%s differs from golden data' % test_file)

    def run_compositor(self, args, fail_ok = False):
        all_args = [
            '../compositor',
            '--config', 'COMPOSITOR_SCHEMA', '../compositor_schema.json',
            ] + args

        cmd = ' '.join(all_args)
        print cmd
        
        filename_out = 'test_%d.stdout' % os.getpid()
        fd_out = open(filename_out,'w')
        filename_err = 'test_%d.stderr' % os.getpid()
        fd_err = open(filename_err,'w')

        rc = subprocess.call(all_args, stdout=fd_out, stderr=fd_err)

        fd_out = None
        fd_err = None
        
        out = open(filename_out).read()
        err = open(filename_err).read()
        os.unlink(filename_out)
        os.unlink(filename_err)

        if not fail_ok and rc != 0:
            self.fail('compositor failure: %s' % err)
        
        return (rc, out, err)


    def test_small_darkest_gray(self):
        test_file = self.make_file(TEMPLATE_GRAY)
        quality_out = 'sd_quality_out.tif'

        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-o', test_file, 
            '-qo', quality_out,
            '-i', 
            self.make_file(TEMPLATE_GRAY, [[0, 1], [6, 5]]),
            '-i',
            self.make_file(TEMPLATE_GRAY, [[9, 8], [2, 3]]),
            ]

        self.run_compositor(args)

        self.compare_file(test_file, [[0, 1], [2, 3]])
        self.compare_file(quality_out, 
                          [[[1.0, 0.99609375],
                            [0.9921875, 0.98828125]],
                           [[1.0, 0.99609375],
                            [0.9765625, 0.98046875]],
                           [[0.96484375, 0.96875],
                            [0.9921875, 0.98828125]]],
                          tolerance = 0.000001)

        os.unlink('sd_quality_out.tif')
        self.clean_files()
        
    def test_small_darkest_gray_json(self):
        json_file = 'small_darkest_gray.json'
        test_file = self.make_file(TEMPLATE_GRAY)
        quality_out = 'sdj_quality_out.tif'

        control = {
            'output_file': test_file,
            'quality_output': quality_out,
            'compositors': [
                {
                    'class': 'darkest',
                    'scale_min': 0.0,
                    'scale_max': 255.0,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY, [[0, 1], [6, 5]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, [[9, 8], [2, 3]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor([ '-q', '-j', json_file])

        self.compare_file(test_file, [[0, 1], [2, 3]])
        self.compare_file(quality_out,
                          [[[1.0, 0.9960784316062927],
                            [0.9921568632125854, 0.9882352948188782]],
                           [[1.0, 0.9960784316062927],
                            [0.9764705896377563, 0.9803921580314636]],
                           [[0.9647058844566345, 0.9686274528503418],
                            [0.9921568632125854, 0.9882352948188782]]],
                          tolerance = 0.000001)

        os.unlink('sdj_quality_out.tif')
        os.unlink(json_file)
        self.clean_files()
        
    def test_small_darkest_rgb(self):
        test_file = self.make_file(TEMPLATE_RGB)
        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-o', test_file, 
            '-i',
            self.make_file(TEMPLATE_RGB, 
                           [[[9, 1], [3, 0]],
                            [[0, 1], [3, 0]],
                            [[0, 1], [3, 9]]]),
            '-i',
            self.make_file(TEMPLATE_RGB, 
                           [[[0, 8], [6, 5]],
                            [[9, 8], [6, 5]],
                            [[9, 8], [6, 5]]]),
            ]

        self.run_compositor(args)

        self.compare_file(test_file, 
                          [[[9, 1], [3, 0]], 
                           [[0, 1], [3, 0]], 
                           [[0, 1], [3, 9]]])

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
        
    def test_percentile(self):
        test_file = self.make_file(TEMPLATE_GRAY)
        
        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-s', 'quality_percentile', '60.0',
            '-o', test_file, 
            '-i', self.make_file(TEMPLATE_GRAY, [[9, 0], [1, 1]]),
            '-i', self.make_file(TEMPLATE_GRAY, [[5, 1], [9, 9]]),
            '-i', self.make_file(TEMPLATE_GRAY, [[1, 2], [9, 1]]),
            ]

        self.run_compositor(args)

        self.compare_file(test_file, [[5, 1], [9, 1]])

        self.clean_files()
        
    def test_percentile_json(self):
        json_file = 'percentile.json'
        test_file = self.make_file(TEMPLATE_GRAY)

        control = {
            'output_file': test_file,
            'compositors': [
                {
                    'class': 'darkest',
                    },
                {
                    'class': 'percentile',
                    'quality_percentile': 60.0,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY, [[9, 0], [1, 1]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, [[5, 1], [9, 9]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, [[1, 2], [9, 1]]),
                    },
                ],
            }

        open(json_file, 'w').write(json.dumps(control))
        self.run_compositor(['-q', '-j', json_file])
        self.compare_file(test_file, [[5, 1], [9, 1]])

        os.unlink(json_file)
        self.clean_files()
        
    def test_quality_file(self):
        test_file = self.make_file(TEMPLATE_GRAY)
        quality_out = 'qf_test_quality.tif'

        in_1 = self.make_file(TEMPLATE_GRAY, [[101, 101], [101, 101]])
        self.make_file(TEMPLATE_FLOAT, [[0.5, 2.0], [-1.0, 0.01]],
                       filename = in_1 + '.q')

        in_2 = self.make_file(TEMPLATE_GRAY, [[102, 102], [102, 102]])
        self.make_file(TEMPLATE_FLOAT, [[2.0, 1.8], [-1.0, 0.25]],
                       filename = in_2 + '.q')
                       
        args = [
            '-q',
            '-s', 'quality', 'darkest',
            '-qo', quality_out,
            '-s', 'quality_file', '.q',
            '-s', 'quality_file_scale_min', '0.0',
            '-s', 'quality_file_scale_max', '2.0',
            '-o', test_file, 
            '-i', in_1,
            '-i', in_2,
            ]

        self.run_compositor(args)

        self.compare_file(test_file, [[102, 101], [0, 102]])
        self.compare_file(quality_out, 
                          [[[0.6015625, 0.60546875],
                            [0.0, 0.0751953125]],
                           [[0.1513671875, 0.60546875],
                            [-1.0, 0.0030273436568677425]],
                           [[0.6015625, 0.5414062142372131],
                            [-1.0, 0.0751953125]]],
                          tolerance=0.001)

        os.unlink(quality_out)
        self.clean_files()
        
    def test_quality_file_json(self):
        json_file = 'quality_file.json'
        test_file = self.make_file(TEMPLATE_GRAY)
        quality_out = 'qfj_test_quality.tif'

        control = {
            'output_file': test_file,
            'quality_output': quality_out,
            'compositors': [
                {
                    'class': 'darkest',
                    'scale_min': 0.0,
                    'scale_max': 255.0,
                    },
                {
                    'class': 'qualityfromfile',
                    'file_key': 'quality',
                    'scale_min': 0.0,
                    'scale_max': 2.0,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[101, 101], [101, 101]]),
                    'quality': self.make_file(TEMPLATE_FLOAT, 
                                              [[0.5, 2.0], [-1.0, 0.01]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[102, 102], [102, 102]]),
                    'quality': self.make_file(TEMPLATE_FLOAT,
                                              [[2.0, 1.8], [-1.0, 0.25]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor(['-q', '-j', json_file])

        self.compare_file(test_file, [[102, 101], [0, 102]])
        self.compare_file(quality_out, 
                          [[[0.6000000238418579, 0.6039215922355652],
                            [0.0, 0.07500000298023224]],
                           [[0.1509803980588913, 0.6039215922355652],
                            [-1.0, 0.0030196078587323427]],
                           [[0.6000000238418579, 0.5400000214576721],
                            [-1.0, 0.07500000298023224]]],
                          tolerance=0.001)

        os.unlink(quality_out)
        os.unlink(json_file)
        self.clean_files()
        
    def test_snow_quality(self):
        json_file = 'quality_file.json'
        test_file = self.make_file(TEMPLATE_GRAY)
        quality_out = 'snow_test_quality.tif'
        
        snow_only = 23552
        cloud_only = 53248
        cloud_and_snow = 56320
        clear = 20480
        cirrus = 28672

        control = {
            'output_file': test_file,
            'quality_output': quality_out,
            'compositors': [
                {"class": "landsat8",
                 "fully_confident_cloud": 0.01,
                 },
                {"class": "landsat8snow",
                 "fully_confident_snow": 0.02,
                 },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[101, 101], [101, 101]]),
                    'cloud_file': self.make_file(TEMPLATE_UINT16, 
                                              [[clear, snow_only], 
                                               [cloud_and_snow, cloud_only]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[102, 102], [102, 102]]),
                    'cloud_file': self.make_file(TEMPLATE_UINT16,
                                              [[cirrus, cloud_only], 
                                               [cloud_only, cirrus]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[103, 103], [103, 103]]),
                    'cloud_file': self.make_file(TEMPLATE_UINT16,
                                              [[snow_only, cloud_and_snow], 
                                               [clear, snow_only]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor(['-q', '-j', json_file])

        self.compare_file(test_file, [[101, 101], [103, 102]])
	self.compare_file(quality_out,
                          [[[0.528,   0.01056],
                            [0.528,   0.132]],
                           [[0.528,   0.01056],
                            [0.00016, 0.008]],
                           [[0.132,   0.008],
                            [0.008,   0.132]],
                           [[0.01056, 0.00016],
                            [0.528,   0.01056]]], 
                           tolerance=0.001)

        os.unlink(quality_out)
        os.unlink(json_file)
        self.clean_files()
        
    def test_same_source_json(self):
        json_file = 'same_source.json'
        test_file = self.make_file(TEMPLATE_GRAY_3X3)
        quality_out = 'qfj_test_same_source.tif'

        control = {
            'output_file': test_file,
            'quality_output': quality_out,
            'compositors': [
                {
                    'class': 'qualityfromfile',
                    'file_key': 'quality',
                    'scale_min': 0.0,
                    'scale_max': 1.0,
                    },
                {
                    'class': 'samesource',
                    'mismatch_penalty': 0.3,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY_3X3, 
                                               [[101, 101, 101],
                                                [101, 101, 101],
                                                [101, 101, 101]]),
                    'quality': self.make_file(TEMPLATE_FLOAT_3X3, 
                                              [[1.0, 1.0, 1.0],
                                               [0.9, 0.9, 0.9],
                                               [0.4, 0.4, 0.4]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY_3X3, 
                                               [[102, 102, 102],
                                                [102, 102, 102],
                                                [102, 102, 102]]),
                    'quality': self.make_file(TEMPLATE_FLOAT_3X3, 
                                              [[0.9, 0.9, 0.9],
                                               [1.0, 1.0, 1.0],
                                               [1.0, 1.0, 1.0]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor(['-q', '-j', json_file])

        self.compare_file(test_file, [[101, 101, 101],
                                      [101, 101, 101],
                                      [102, 102, 102]])
        self.compare_file(quality_out, 
                          [[[1.0, 1.0, 1.0],
                            [0.9, 0.9, 0.9],
                            [0.8, 0.7, 0.8]],
                           [[1.0, 1.0, 1.0],
                            [0.9, 0.9, 0.9],
                            [0.4, 0.4, 0.4]],
                           [[0.9, 0.9, 0.9],
                            [0.8, 0.7, 0.8],
                            [0.8, 0.7, 0.8]]],
                          tolerance=0.001)

        os.unlink(quality_out)
        os.unlink(json_file)
        self.clean_files()
        
    def test_sieve_json(self):
        json_file = 'sieve.json'
        test_file = self.make_file(TEMPLATE_GRAY_3X3)
        st_out = 'st_test_same_source.tif'

        control = {
            'output_file': test_file,
            'source_sieve_threshold': 2,
            'source_trace': st_out,
            'compositors': [
                {
                    'class': 'qualityfromfile',
                    'file_key': 'quality',
                    'scale_min': 0.0,
                    'scale_max': 1.0,
                    }
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY_3X3, 
                                               [[101, 101, 101],
                                                [101, 101, 101],
                                                [101, 101, 101]]),
                    'quality': self.make_file(TEMPLATE_FLOAT_3X3, 
                                              [[1.0, 0.0, 1.0],
                                               [0.0, 0.0, 0.0],
                                               [1.0, 1.0, 1.0]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY_3X3, 
                                               [[102, 102, 102],
                                                [102, 102, 102],
                                                [102, 102, 102]]),
                    'quality': self.make_file(TEMPLATE_FLOAT_3X3, 
                                              [[0.5, 0.5, 0.5],
                                               [0.5, 0.5, 0.5],
                                               [0.5, 0.5, 0.5]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor(['-q', '-j', json_file])

        self.compare_file(test_file, [[102, 102, 102],
                                      [102, 102, 102],
                                      [101, 101, 101]])

        os.unlink(st_out)
        os.unlink(json_file)
        self.clean_files()
        
    def test_schema_validation(self):
        test_file = self.make_file(TEMPLATE_GRAY)
        json_file = 'schema_test.json'
        control = {
            'output_file': test_file,
            'compositors': [
                {
                    'classx': 'darkest',
                    'scale_min': 0.0,
                    'scale_max': 255.0,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[101, 101], [101, 101]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[102, 102], [102, 102]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        (rc, out, err) = self.run_compositor(['-q', '-j', json_file],
                                             fail_ok = True)
        
        self.assertNotEqual(0, rc)
        self.assertEqual('', out)
        self.assertIn("required item 'class' not found", err)

        os.unlink(json_file)
        self.clean_files()
        
    def test_small_darkest_gray_averaging_json(self):
        json_file = 'small_darkest_gray_averaging.json'
        test_file = self.make_file(TEMPLATE_GRAY)
        control = {
            'output_file': test_file,
            'average_best_ratio': 0.75,
            'compositors': [
                {
                    'class': 'darkest',
                    'scale_min': 0.0,
                    'scale_max': 255.0,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[10, 255], [6, 5]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[20, 255], [2, 3]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_GRAY, 
                                               [[30, 12], [2, 3]]),
                    },
                ],
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor([ '-q', '-j', json_file])

        self.compare_file(test_file, [[15, 12], [2, 3]])

        os.unlink(json_file)
        self.clean_files()
        
    def test_darkest_alt_rgb_ratio_json(self):
        json_file = 'small_darkest_alt_rgb_ratio.json'
        test_file = self.make_file(TEMPLATE_RGB)

        control = {
            'output_file': test_file,
            'compositors': [
                {
                    'class': 'darkest',
                    'scale_min': 0.0,
                    'scale_max': 255.0,
                    'band_1_weight': 0.9,
                    'band_2_weight': 0.05,
                    'band_3_weight': 0.05,
                    },
                ],
            'inputs': [
                {
                    'filename': self.make_file(TEMPLATE_RGB, 
                                               [[[255, 12], [0, 11]],
                                                [[10, 200], [100, 5]],
                                                [[10, 200], [200, 5]]]),
                    },
                {
                    'filename': self.make_file(TEMPLATE_RGB, 
                                               [[[10, 255], [6, 5]],
                                                [[10,  30], [6, 5]],
                                                [[10,  30], [6, 5]]]),
                    },
                ]
            }

        open(json_file,'w').write(json.dumps(control))
        self.run_compositor([ '-q', '-j', json_file])

        self.compare_file(
                test_file, [
                    [[10, 12], [6, 5]],
                    [[10, 200], [6, 5]],
                    [[10, 200], [6, 5]],
                    ])
        os.unlink(json_file)
        self.clean_files()
        
if __name__ == '__main__':
    unittest.main()
