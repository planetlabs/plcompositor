import gviewapp
import gtk
import os
import gdal
import string
import gvplot
import toolexample
import Numeric

FALSE = gtk.FALSE
TRUE = gtk.TRUE
import gview,gvutils

import ev_mosaic_viewer

global last_plot_win

last_plot_win = None

def graph(view_xy):
    global last_plot_win

    #last_plot_win = graph_rgb(view_xy)
    new_plot_win = graph_darkness(view_xy)
    
    if new_plot_win is not None and last_plot_win is not None:
        try:
            last_plot_win.close()
        except:
            pass
    last_plot_win = new_plot_win
        
def graph_rgb(view_xy):
    view = gview.app.view_manager.get_active_view_window()
    layers = view.viewarea.list_layers()
    to_plot = []
    for layer in layers:
        lclass = ev_mosaic_viewer.layer_class(layer)
        raster = layer.get_data(0)
        date = get_dataset_date(raster.get_dataset())
        if date is None:
            continue
        
        pixel_xy = layer.view_to_pixel(view_xy[0], view_xy[1])

        data_point = [date]
        
        for comp in range(3):
            raster = layer.get_data(comp)
            data_point.append(raster.get_sample(pixel_xy[0], pixel_xy[1]))
        
        print raster.get_dataset().GetDescription(), data_point

        to_plot.append(data_point)

    to_plot = Numeric.array(to_plot)
    
    return gvplot.plot(to_plot,
                       xaxis = 'Days',
                       yaxis = 'Value',
                       ymin = 6500.0,
                       ymax = 11000.0,
                       terminal='openev',
                       multiplot=True)

def graph_darkness(view_xy):
    view = gview.app.view_manager.get_active_view_window()
    layers = view.viewarea.list_layers()
    to_plot = []
    for layer in layers:
        lclass = ev_mosaic_viewer.layer_class(layer)
        if lclass != ev_mosaic_viewer.LYR_LANDSAT8:
            continue
        
        raster = layer.get_data(0)

        # ignore rasters that aren't source scenes with a date.
        date = get_dataset_date(raster.get_dataset())
        if date is None:
            continue
        
        pixel_xy = layer.view_to_pixel(view_xy[0], view_xy[1])

        data_point = [date]

        for xoff in range(-2,4,2):
            for yoff in range(-2,4,2):
                rgb_sum = 0
                for comp in range(3):
                    raster = layer.get_data(comp)
                    rgb_sum += raster.get_sample(pixel_xy[0] + xoff,
                                                 pixel_xy[1] + yoff)

                rgb_sum *= 0.3333

                to_plot.append(rgb_sum)

    to_plot.sort()
    
    return gvplot.plot(to_plot,
                       xaxis = 'Sample',
                       yaxis = 'Brightness',
                       ymin = 6500.0,
                       ymax = 11000.0,
                       terminal='openev')

def get_dataset_date(dataset = None, name = None):
    if dataset is not None:
        name = dataset.GetDescription()
        
    lc8 = name.find('LC8')
    if lc8 == -1:
        print 'unable to parse date from %s' % name
        return None

    year = int(name[lc8+9:lc8+13])
    doy = int(name[lc8+13:lc8+16])
    return (year - 2013)*365 + doy

