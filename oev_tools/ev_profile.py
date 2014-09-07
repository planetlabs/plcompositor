import gviewapp
import gtk
import os
import gdal
import string
import gvplot

FALSE = gtk.FALSE
TRUE = gtk.TRUE
import gview,gvutils

ProfileWindow = None

def graph(self, view_xy):
    global ProfileWindow

    view = gview.app.view_manager.get_active_view_window()
    layers = view.viewarea.list_layers()
    to_plot = []
    for layer in layers:
        lclass = layer_class(layer)
        raster = layer.get_data(0)
        date = self.get_dataset_date(raster.get_dataset())
        pixel_xy = layer.view_to_pixel(view_xy[0], view_xy[1])
        value = raster.get_sample(pixel_xy[0], pixel_xy[1])
        
        print raster.get_dataset().GetDescription(), date, value
        to_plot.append([date, value])

    gvplot.plot(to_plot,
                xaxis = 'Days',
                yaxis = 'Value',
                terminal='openev')
        
def get_dataset_date(self, dataset = None, name = None):
    if dataset is not None:
        name = dataset.GetDescription()
        
    lc8 = name.find('LC8')
    if lc8 == -1:
        print 'unable to parse date from %s' % name
        return None

    year = int(name[lc8+9:lc8+13])
    doy = int(name[lc8+13:lc8+16])
    return (year - 2013)*365 + doy

class Histogram_ToolDlg(toolexample.General_ROIToolDlg):
    def __init__(self):
        toolexample.General_ROIToolDlg.__init__(self)

    def init_setup_window(self):
        self.set_title('Quality Histogram Tool')
        self.set_border_width(10)

    def init_customize_gui_panel(self):
        # Inherit all the usual stuff...
        toolexample.General_ROIToolDlg.init_customize_gui_panel(self)

        # Add new frame with pixel info, keeping track of
        # the frame and text object...
        self.frame_dict['histogram_frame'] = gtk.GtkFrame()
        self.show_list.append(self.frame_dict['histogram_frame'])
        
        # Initialize with dummy histogram.
        
        histogram = []
        for i in range(256):
            histogram.append( 0 )

        (pm, mask) = self.get_histview(histogram,0,256,-1,1)
        self.viewarea = gtk.GtkPixmap(pm,mask)
        self.entry_dict['histogram_view'] = self.viewarea
        self.show_list.append(self.viewarea)
        self.frame_dict['histogram_frame'].add(self.viewarea)

        self.main_panel.pack_start(self.frame_dict['histogram_frame'],
                                   gtk.TRUE,gtk.TRUE,0)   

    def set_entry_sensitivities(self,bool_val):
        self.entry_dict['start_line'].set_sensitive(bool_val)
        self.entry_dict['start_pix'].set_sensitive(bool_val)
        self.entry_dict['num_lines'].set_sensitive(bool_val)
        self.entry_dict['num_pix'].set_sensitive(bool_val)

    def activate_toggled(self,*args):
        if self.button_dict['Activate'].get_active():
            self.set_entry_sensitivities(gtk.TRUE)
            self.button_dict['Auto Update'].set_sensitive(gtk.TRUE)
            self.button_dict['Auto Update'].set_active(gtk.FALSE)
            self.button_dict['Analyze'].set_sensitive(gtk.TRUE)
            self.button_dict['Set Tool'].set_sensitive(gtk.TRUE)
            self.notify('re-activated')
        else:
            self.set_entry_sensitivities(gtk.FALSE)
            self.button_dict['Auto Update'].set_active(gtk.FALSE)
            self.button_dict['Auto Update'].set_sensitive(gtk.FALSE)
            self.button_dict['Analyze'].set_sensitive(gtk.FALSE)
            self.button_dict['Set Tool'].set_sensitive(gtk.FALSE)
       
    def get_histview( self, histogram, min, max,set_ymin=None,set_ymax=None ):

        # Prepare histogram plot

        data = []
        bucket_width = (max - min) / 256.0
        for bucket_index in range(256):
            data.append( (min + bucket_width * bucket_index,
                          histogram[bucket_index]) )

        # Generate histogram graph
        
        import gvplot

        xpm_file = gvutils.tempnam(extension='xpm')
        if ((set_ymin is not None) and (set_ymax is not None)):
            # Allows you to explicitly set ymin/ymax at startup
            # to avoid the gnuplot 0 y-range warning.
            gvplot.plot( data, xaxis = 'Pixel Value', yaxis = 'Count',
                         xmin = min, xmax = max, ymin = set_ymin,
                         ymax = set_ymax, output = xpm_file,
                         title = 'Histogram', terminal = 'xpm' )
        else:
            gvplot.plot( data, xaxis = 'Pixel Value', yaxis = 'Count',
                         xmin = min, xmax = max, output = xpm_file,
                         title = 'Histogram', terminal = 'xpm' )

        # apply it to display.

        gdk_pixmap, gdk_mask = \
              gtk.create_pixmap_from_xpm(self,None,xpm_file)

        os.unlink( xpm_file )

        return (gdk_pixmap, gdk_mask)
