###############################################################################
# $Id: gvrastertools.py,v 1.5 2004/01/12 16:54:56 warmerda Exp $
#
# Project:  OpenEV
# Purpose:  Raster tools for OpenEV built using Gillian's Tool Architecture.
#           Currently just Histogram.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
# Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
###############################################################################
# 
#  $Log: gvrastertools.py,v $
#  Revision 1.5  2004/01/12 16:54:56  warmerda
#  Properly apply scaling for histogram data.
#  http://bugzilla.remotesensing.org/show_bug.cgi?id=457
#
#  Revision 1.4  2002/07/07 21:08:18  warmerda
#  avoid depending on Numeric at the top level
#
#  Revision 1.3  2002/04/25 15:19:58  gmwalter
#  Updated the menu extension mechanism to get rid of the extend_menu function and to allow the help menu to remain on the right side of the menubar.  Got rid of some extra print statements and the gnuplot 0 y-range warning in the histogram tool initialization.
#
#  Revision 1.2  2002/04/18 20:36:09  warmerda
#  ensure the histogram dialog gets initialized
#
#  Revision 1.1  2002/04/18 20:23:24  warmerda
#  New
#

import gviewapp
import gtk
import os
import gdal
import string
import gvsignaler
import toolexample

FALSE = gtk.FALSE
TRUE = gtk.TRUE
import gview,gvutils


class QualityHistogramROITool(toolexample.GeneralROITool):

    def __init__(self,app=None):
        toolexample.GeneralROITool.__init__(self,app)

    def init_dialog(self):
        self.RP_ToolDlg = Histogram_ToolDlg()

    def init_menu(self):
        pass

    def update_dlgroi_frame(self,*args):
        # Update frame as in general tool
        if self.RP_ToolDlg.is_active():       
            roi_info = self.RP_Stored.roi
            self.RP_ToolDlg.update_roiframe(roi_info)

        else:
            return

        self.update_roi_view()

        if self.RP_ToolDlg.is_auto_updating():
            # Automatic analysis is on
            self.RP_ToolDlg.analyze_cb(self)

    def update_roi_view(self,*args):
        pass
    
    def analyze_cb(self,*args):
        # Find the view and layer
        cview = self.app.view_manager.get_active_view_window().title
        clayer = self.app.sel_manager.get_active_layer()
        if (cview is None) or (clayer is None):
            # analysis only makes sense in the context of a view and layer
            gvutils.error('No View/Layer selected!')
            return

        text = self.basic_region_analysis(cview,clayer) 

    def basic_region_analysis(self,cview,clayer):
	import Numeric, gdalnumeric

        line = int(self.RP_ToolDlg.entry_dict['start_line'].get_text())
        pix = int(self.RP_ToolDlg.entry_dict['start_pix'].get_text())
        sl =  int(self.RP_ToolDlg.entry_dict['num_lines'].get_text())
        sp =  int(self.RP_ToolDlg.entry_dict['num_pix'].get_text())

        clayer = self.app.sel_manager.get_active_layer()
        if clayer is None:
            # Target can only be extracted if a layer is selected
            return 'No selected raster layer'

        dsb = clayer.get_data(0).get_band()

        # Modify default to be whole image.
        if sl is 1 and sp is 1 and line is 1 and pix is 1:
            sl = dsb.YSize - line + 1
            sp = dsb.XSize - pix + 1

        # Create a temporary band for sampling if we have a subrect.
        temp_copy = 0
        if line != 1 or pix != 1 or sp != dsb.XSize or sl != dsb.YSize:
            # print line, pix, sp, sl
            temp_copy = 1
            filename = clayer.get_parent().get_dataset().GetDescription()
            target_data = gdalnumeric.LoadFile(filename,pix-1,line-1,sp,sl)
            target_ds = gdalnumeric.OpenArray(target_data)
            dsb = target_ds.GetRasterBand(1)

        # Compute the histogram.
        
        x_min = clayer.min_get(0)
        x_max = clayer.max_get(0)
        histogram = dsb.GetHistogram( x_min, x_max, 256, 1, 0 )

        y_min = 0
        y_max = max(histogram[1:-1])

        (pm, mask) = self.RP_ToolDlg.get_histview( histogram, x_min, x_max,
                                                   y_min, y_max)
        
        self.RP_ToolDlg.viewarea.set(pm,mask)
        
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
