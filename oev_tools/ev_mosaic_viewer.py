
from gtk import *

import sys
import gview
import string           
import gvutils
import GtkExtra
import time
import os
import gviewapp
import gvplot

import gdal

sys.path.append('.')

import ev_profile
from quality_hist_tool import QualityHistogramROITool


LYR_GENERIC  = 0
LYR_LANDSAT8 = 1
LYR_SOURCE_TRACE = 2
LYR_QUALITY = 3

def layer_class(layer):
    dataset = layer.get_parent().get_dataset()
    if (dataset.GetRasterBand(1).DataType == gdal.GDT_UInt16 or dataset.GetRasterBand(1).DataType == gdal.GDT_Int16) and dataset.RasterCount == 4:
        return LYR_LANDSAT8

    if dataset.GetDescription().find('st-') != -1:
        return LYR_SOURCE_TRACE

    if dataset.RasterCount > 4 and dataset.GetRasterBand(1).DataType == gdal.GDT_Float32:
        return LYR_QUALITY

    return LYR_GENERIC

class MosaicViewerTool(gviewapp.Tool_GViewApp):
    
    def __init__(self,app=None):
        gviewapp.Tool_GViewApp.__init__(self,app)
        self.init_menu()
        self.hist_tool = QualityHistogramROITool(app)
        self.graphing = False

    def launch_dialog(self,*args):
        self.win = MosaicDialog(app=gview.app, tool=self)
        self.win.show()
        self.win.rescale_landsat_cb()
        self.win.gui_refresh()
        self.track_view_activity()

    def key_down_cb(self, viewarea, event):
        try:
            print 'down %s/%d' % (chr(event.keyval), event.keyval)
        except:
            print 'down <undefined>/%d' % event.keyval
        if event.keyval == ord('g'):
            print 'enable graphing'
            self.graphing = True

    def key_up_cb(self, viewarea, event):
        try:
            print 'up %s/%d' % (chr(event.keyval), event.keyval)
        except:
            print 'up <undefined>/%d' % event.keyval
        if event.keyval == ord('g'):
            print 'disable graphing'
            self.graphing = False

    def mouse_cb(self, viewarea, event):
        print 'mouse event:', event.type

        if event.type == 4:
            print event.type, event.button, event.state
        elif event.type == 3:
            #print event.x, event.y
            #print viewarea.map_pointer((event.x, event.y))
            if self.graphing:
                ev_profile.graph(viewarea.map_pointer((event.x, event.y)))

    def track_view_activity(self):
        view = gview.app.view_manager.get_active_view_window()
        view.viewarea.connect('key-press-event', self.key_down_cb)
        view.viewarea.connect('key-release-event', self.key_up_cb)
        view.viewarea.connect('motion-notify-event', self.mouse_cb)
        view.viewarea.connect('button-press-event', self.mouse_cb)

    def init_menu(self):
        self.menu_entries.set_entry("Tools/Mosaic Viewer",2,
                                    self.launch_dialog)

class MosaicDialog(GtkWindow):

    def __init__(self,app=None, tool=None):
        self.tool = tool
        self.updating = False
        GtkWindow.__init__(self)
        self.quality_layer = None
        self.set_title('Mosaic Viewer')
        self.create_gui()
        self.show()
        self.gui_refresh()

    def show(self):
        GtkWindow.show_all(self)

    def close(self, *args):
        self.hide()
        self.visibility_flag = 0
        return TRUE

    def set_quality_band_cb(self,*args):
        if self.updating or self.quality_layer is None:
            return

        try:
            scale_min = float(self.min_entry.get_text())
        except:
            scale_min = 0.0
        try:
            scale_max = float(self.max_entry.get_text())
        except:
            scale_max = 1.0;

        dataset = self.quality_layer.get_parent().get_dataset()

        new_select = None
        new_text = self.band_combo.entry.get_text()
        for i in range(len(self.quality_band_names)):
            if new_text == self.quality_band_names[i]:
                new_select = i+1

        raster = gview.manager.get_dataset_raster( dataset, new_select)
        for isrc in range(3):
            self.quality_layer.set_source(isrc, raster, scale_min, scale_max)
        self.tool.hist_tool.analyze_cb()

    def quality_refresh(self):
        assert self.quality_layer is not None

        dataset = self.quality_layer.get_parent().get_dataset()

        self.quality_band_names = []
        for band_num in range(1,dataset.RasterCount):
            self.quality_band_names.append(
                dataset.GetRasterBand(band_num).GetMetadata()['DESCRIPTION'])
            
        self.band_combo.set_popdown_strings( self.quality_band_names)

    def gui_refresh(self):
        if self.quality_layer is not None:
            self.quality_refresh()

    def adjustment_cb(self,adjustment,*args):
        if self.updating or self.quality_layer is None:
            return

        value = adjustment.value

        if adjustment == self.min_adjustment:
            self.min_entry.set_text(str(value))
        else:
            self.max_entry.set_text(str(value))
        self.set_quality_band_cb()

    def entry_cb(self,entry,*args):
        if self.updating:
            return

        self.set_quality_band_cb()

    def find_tool(self, tool_name):
        for (name, tool_inst) in gview.app.Tool_List:
            if name == tool_name:
                return tool_inst
        return None

    def create_gui(self):


        vbox = GtkVBox(spacing=5)
        vbox.set_border_width(10)
        self.add(vbox)
        
        # Add the Quality Band Selection Combo
        hbox = GtkHBox(spacing=5)
        vbox.pack_start(hbox,expand=FALSE)
        hbox.pack_start(GtkLabel('Quality:'), expand=FALSE)
        self.band_combo = GtkCombo()
        hbox.pack_start(self.band_combo)
        self.band_combo.entry.connect('changed', self.set_quality_band_cb)
        self.band_combo.set_popdown_strings( 
            ['XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'])

        band_list = ['inactive']
        self.band_combo.set_popdown_strings( band_list )

        # ------ Quality Scale Min -------
        hbox = GtkHBox(spacing=5)
        vbox.pack_start(hbox)
        hbox.pack_start(GtkLabel('Scale Min:'),expand=FALSE)
        self.min_adjustment = GtkAdjustment(0.0, 0.0, 1.0, 0.05, 0.05, 0.05)
        self.min_adjustment.connect('value-changed',self.adjustment_cb)
        self.min_slider = GtkHScale(self.min_adjustment)
        self.min_slider.set_digits(3)
        hbox.pack_start(self.min_slider)
        self.min_entry = GtkEntry(maxlen=8)
        self.min_entry.connect('activate',self.entry_cb)
        self.min_entry.connect('leave-notify-event',self.entry_cb)
        self.min_entry.set_text('0.0')
        hbox.pack_start(self.min_entry,expand=FALSE)

        # ------ Quality Scale Max -------
        hbox = GtkHBox(spacing=5)
        vbox.pack_start(hbox)
        hbox.pack_start(GtkLabel('Scale Max:'),expand=FALSE)
        self.max_adjustment = GtkAdjustment(1.0, 0.0, 1.0, 0.05, 0.05, 0.05)
        self.max_adjustment.connect('value-changed',self.adjustment_cb)
        self.max_slider = GtkHScale(self.max_adjustment)
        self.max_slider.set_digits(3)
        hbox.pack_start(self.max_slider)
        self.max_entry = GtkEntry(maxlen=8)
        self.max_entry.connect('activate',self.entry_cb)
        self.max_entry.connect('leave-notify-event',self.entry_cb)
        self.max_entry.set_text('1.0')
        hbox.pack_start(self.max_entry,expand=FALSE)

        # Add the Rescale and Close action buttons.
	box2 = GtkHBox(spacing=10)
        vbox.add(box2)
        box2.show()

        execute_btn = GtkButton("Histogram")
        execute_btn.connect("clicked", self.tool.hist_tool.roipoitool_cb)
	box2.pack_start(execute_btn)
        
        execute_btn = GtkButton("Rescale")
        execute_btn.connect("clicked", self.rescale_landsat_cb)
	box2.pack_start(execute_btn)
        
        close_btn = GtkButton("Close")
        close_btn.connect("clicked", self.close)
        box2.pack_start(close_btn)

    def rescale_landsat_cb( self, *args ):
        view = gview.app.view_manager.get_active_view_window()
        layers = view.viewarea.list_layers()
        for layer in layers:
            lclass = layer_class(layer)
            if lclass == LYR_LANDSAT8:
                for isrc in range(3):
                    layer.set_source(isrc, layer.get_data(isrc),
                                     4000, 12000)

                dataset = layer.get_parent().get_dataset()
                alpha_raster = gview.manager.get_dataset_raster( dataset, 4 )
                layer.set_source(3, alpha_raster, 0 , 255)
            if lclass == LYR_QUALITY:
                if self.quality_layer is None:
                    self.quality_layer = layer
                    layer.set_source(0, layer.get_data(0), 0.0, 1.0)
                    layer.set_source(1, layer.get_data(0), 0.0, 1.0)
                    layer.set_source(2, layer.get_data(0), 0.0, 1.0)

        self.gui_refresh()
                

TOOL_LIST = ['MosaicViewerTool']

