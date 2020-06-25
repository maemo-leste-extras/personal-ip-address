/*
 * This file is part of Personal IP Address.
 *
 * Copyright (C) 2009 Andrew Olmsted. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
*  
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/* personal-ip-address.c */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "personal-ip-address.h"
#include <hildon/hildon.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <conic.h>
#include <dbus/dbus-glib.h>
#include <osso-ic-dbus.h>

#define HOME_DIR g_get_home_dir()

#define IP_ADDR "/bin/ip -4 address show "
#define IP_COMMAND " | grep \"inet\" | awk -F' ' '{print $2}' | awk -F/ '{print $1}' "
#define INTERFACE_COMMAND "/bin/ip -4 route show default | awk -F'dev' '{print $2}' | awk -F' ' '{print $1}' "

#define _tran(String) dgettext("hildon-libs", String)
#define _(String) gettext (String)

#define PERSONAL_IP_ADDRESS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj,\
							                         PERSONAL_TYPE_IP_ADDRESS,\
							                         PersonalIpAddressPrivate))

struct _PersonalIpAddressPrivate
{
	GtkWidget *homeWidget;
	GtkWidget *event;
	GtkWidget *contents;
	
	GtkWidget *ipImage;
	GtkWidget *ipLabel;
	GtkWidget *ipContent;
	GtkWidget *interfaceContent;
	GtkWidget *networkContent;
	
	gboolean connectOnPress;
	gboolean disconnectOnPress;
	gboolean compactMode;
	gboolean showInterface;
	gboolean isPressed;
	gboolean isConnected;
	
	ConIcConnection *connection;

	DBusGConnection *dbus_connection;
	DBusGProxy *dbus_icd2_proxy;
	
	guint eventID;
};

HD_DEFINE_PLUGIN_MODULE (PersonalIpAddress, personal_ip_address, HD_TYPE_HOME_PLUGIN_ITEM);

//gboolean personal_ip_address_update_content (PersonalIpAddress *self);

void personal_ip_address_read_settings ( PersonalIpAddress *self )
{
	//g_warning("personal_ip_address_read_settings");	
    gchar *filename;
    gboolean fileExists;
    GKeyFile *keyFile;

    keyFile = g_key_file_new();
    filename = g_strconcat (HOME_DIR, "/.personal_ip_address", NULL);
    fileExists = g_key_file_load_from_file (keyFile, filename, G_KEY_FILE_KEEP_COMMENTS, NULL);

    if (fileExists) {
        GError *error=NULL;

        self->priv->connectOnPress = g_key_file_get_boolean (keyFile, "config", "connectOnPress", &error);
        if (error) {
            self->priv->connectOnPress = FALSE;
            g_error_free (error);
            error = NULL;
        }
        self->priv->disconnectOnPress = g_key_file_get_boolean (keyFile, "config", "disconnectOnPress", &error);
        if (error) {
            self->priv->disconnectOnPress = FALSE;
            g_error_free (error);
            error = NULL;
        }
		self->priv->compactMode = g_key_file_get_boolean (keyFile, "config", "compactMode", &error);
		if (error) {
			self->priv->compactMode = TRUE;
			g_error_free (error);
			error = NULL;
		}
        self->priv->showInterface = g_key_file_get_boolean (keyFile, "config", "showInterface", &error);
        if (error) {
            self->priv->showInterface = TRUE;
            g_error_free (error);
            error = NULL;
        }
    } else {
        self->priv->connectOnPress = FALSE;
        self->priv->disconnectOnPress = FALSE;
		self->priv->compactMode = TRUE;
		self->priv->showInterface = TRUE;
    }

    g_key_file_free (keyFile);
    g_free (filename);
}

void personal_ip_address_write_settings (PersonalIpAddress *self)
{
	//g_warning ("personal_ip_address_write_settings");
	GKeyFile *keyFile;
	gchar *fileData;
	FILE *iniFile;
	gsize size;
	gchar *filename;
	
	keyFile = g_key_file_new();
		
	g_key_file_set_boolean (keyFile, "config", "connectOnPress", self->priv->connectOnPress);
	g_key_file_set_boolean (keyFile, "config", "disconnectOnPress", self->priv->disconnectOnPress);
	g_key_file_set_boolean (keyFile, "config", "compactMode", self->priv->compactMode);
	g_key_file_set_boolean (keyFile, "config", "showInterface", self->priv->showInterface);
	
	filename = g_strconcat (HOME_DIR, "/.personal_ip_address", NULL);
	fileData = g_key_file_to_data (keyFile, &size, NULL);
	iniFile = fopen (filename, "w");
	fputs (fileData, iniFile);
	fclose (iniFile);
	g_key_file_free (keyFile);
	g_free (fileData);
	g_free (filename);
}

void personal_ip_address_button_press (GtkWidget *widget, GdkEventButton *event, PersonalIpAddress *self)
{
	//g_warning ("%s", __PRETTY_FUNCTION__);
	if (self->priv->connectOnPress) {
		self->priv->isPressed = TRUE;
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

void personal_ip_address_button_release (GtkWidget *widget, GdkEventButton *event, PersonalIpAddress *self)
{
	//g_warning ("%s", __PRETTY_FUNCTION__);
	if (self->priv->connectOnPress) {
		self->priv->isPressed = FALSE;
	
		if (self->priv->isConnected && self->priv->disconnectOnPress)
			system ("run-standalone.sh dbus-send --system --dest=com.nokia.icd /com/nokia/icd_ui com.nokia.icd_ui.disconnect boolean:true");
		else
			con_ic_connection_connect (self->priv->connection, CON_IC_CONNECT_FLAG_NONE);
	
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

void personal_ip_address_leave_event (GtkWidget *widget, GdkEventCrossing *event, PersonalIpAddress *self)
{
	//g_warning ("%s", __PRETTY_FUNCTION__);
	if (self->priv->connectOnPress) {
		self->priv->isPressed = FALSE;
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

void personal_ip_address_content_create (PersonalIpAddress *self)
{
	//g_warning ("personal_ip_address_content_create");
	self->priv->contents = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (self->priv->contents), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (self->priv->contents), 0);
	GtkSizeGroup *group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));

	GtkIconTheme *theme = gtk_icon_theme_get_default ();
	if (gtk_icon_theme_has_icon (theme, "personal-ip-address")) {
		self->priv->ipImage = gtk_image_new_from_icon_name ("personal-ip-address", GTK_ICON_SIZE_BUTTON);
	} else {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale ("/usr/share/icons/hicolor/64x64/hildon/personal-ip-address.png", 58, 58, TRUE, NULL);
		self->priv->ipImage = gtk_image_new_from_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}
	self->priv->ipLabel = gtk_label_new (_("IP"));
	GtkWidget *box = gtk_hbox_new (FALSE, 0);
	self->priv->networkContent = gtk_label_new ("");
	self->priv->interfaceContent = gtk_label_new ("");
	self->priv->ipContent = gtk_label_new (_("(no IP found)"));
	hildon_helper_set_logical_font (self->priv->networkContent, "HomeSystemFont");
	gtk_label_set_ellipsize (GTK_LABEL (self->priv->networkContent), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (box), self->priv->ipImage, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), self->priv->ipLabel, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), self->priv->interfaceContent, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), self->priv->networkContent, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), self->priv->ipContent, FALSE, FALSE, 0);
	hildon_helper_set_logical_color (self->priv->ipContent, GTK_RC_FG, GTK_STATE_NORMAL, "ActiveTextColor");
	hildon_helper_set_logical_color (self->priv->interfaceContent, GTK_RC_FG, GTK_STATE_NORMAL, "ActiveTextColor");
	gtk_misc_set_alignment (GTK_MISC (self->priv->ipLabel), 0, 1);
	gtk_misc_set_alignment (GTK_MISC (self->priv->ipContent), 1, 1);
	gtk_misc_set_alignment (GTK_MISC (self->priv->interfaceContent), 0, 1);
	gtk_misc_set_alignment (GTK_MISC (self->priv->networkContent), 0, 1);
	gtk_misc_set_padding (GTK_MISC (self->priv->ipContent), HILDON_MARGIN_DEFAULT, HILDON_MARGIN_HALF);
	gtk_misc_set_padding (GTK_MISC (self->priv->interfaceContent), HILDON_MARGIN_DEFAULT, HILDON_MARGIN_HALF);
	gtk_misc_set_padding (GTK_MISC (self->priv->networkContent), HILDON_MARGIN_DEFAULT, HILDON_MARGIN_HALF);
	gtk_misc_set_padding (GTK_MISC (self->priv->ipLabel), HILDON_MARGIN_DEFAULT, HILDON_MARGIN_HALF);

	gtk_container_add (GTK_CONTAINER (self->priv->contents), box);
	gtk_box_pack_start (GTK_BOX (self->priv->homeWidget), self->priv->contents, FALSE, FALSE, 0);
	
	g_signal_connect (self->priv->contents, "button-release-event", G_CALLBACK (personal_ip_address_button_release), self);
	g_signal_connect (self->priv->contents, "button-press-event", G_CALLBACK (personal_ip_address_button_press), self);
	g_signal_connect (self->priv->contents, "leave-notify-event", G_CALLBACK (personal_ip_address_leave_event), self);
	
    gtk_widget_show_all (self->priv->homeWidget);
	if (self->priv->compactMode) {
		gtk_widget_hide (self->priv->ipImage);
		if (self->priv->showInterface) {
			gtk_widget_show (self->priv->ipLabel);
			gtk_widget_show (self->priv->interfaceContent);
			gtk_widget_hide (self->priv->networkContent);
		} else {
			gtk_widget_hide (self->priv->ipLabel);
			gtk_widget_hide (self->priv->interfaceContent);
			gtk_widget_show (self->priv->networkContent);
		}
	} else {
		gtk_widget_hide (self->priv->ipLabel);
		gtk_widget_show (self->priv->ipImage);
		if (self->priv->showInterface) {
			gtk_widget_show (self->priv->interfaceContent);
			gtk_widget_hide (self->priv->networkContent);
		} else {
			gtk_widget_hide (self->priv->interfaceContent);
			gtk_widget_show (self->priv->networkContent);
		}
	}
}

static void personal_ip_address_on_method_call_finished (DBusGProxy *proxy, DBusGProxyCall *call_id, PersonalIpAddress *self)
{
	gchar *network_id = NULL;
	guint active_time, signal_strength, packets_recieved, packets_sent, bytes_recieved, bytes_sent;

	dbus_g_proxy_end_call (proxy, call_id, NULL, G_TYPE_STRING, &network_id, G_TYPE_UINT, &active_time, G_TYPE_UINT, &signal_strength, G_TYPE_UINT, &packets_recieved, G_TYPE_UINT, &packets_sent, G_TYPE_UINT, &bytes_recieved, G_TYPE_UINT, &bytes_sent, G_TYPE_INVALID);
	if (network_id)
	{
		ConIcIap *iap = con_ic_connection_get_iap (self->priv->connection, network_id);
		const gchar *connection_name = con_ic_iap_get_name (iap);
		//gchar *final_name = g_strconcat (connection_name, " (", gtk_label_get_text (GTK_LABEL (self->priv->interfaceContent)), ")", NULL);
		//if (connection_name)
			gtk_label_set_text (GTK_LABEL (self->priv->networkContent), connection_name);
		g_object_unref (iap);
		g_free (network_id);
		//g_free (final_name);
	}
}

gboolean personal_ip_address_update_content (PersonalIpAddress *self)
{
	//g_warning ("personal_ip_address_update_content");
	FILE *fp;
	gchar line[256];
	gchar *interface;
	gboolean found = FALSE;

	fp = popen (INTERFACE_COMMAND, "r");
	
	while (fgets (line, sizeof line, fp)) {
		interface = g_strstrip (line);
		gtk_label_set_text (GTK_LABEL (self->priv->interfaceContent), interface);
		if (!g_str_equal (interface, "")) {
			found = TRUE;
			break;
		}
	}
	pclose (fp);
	
	if (found) {
		gchar *cmd = g_strconcat (IP_ADDR, interface, IP_COMMAND, NULL);
		found = FALSE;
		fp = popen (cmd, "r");
		
		while (fgets (line, sizeof line, fp)) {
			gtk_label_set_text (GTK_LABEL (self->priv->ipContent), g_strstrip (line));
			found = TRUE;
			break;
		}
		pclose (fp);
		g_free (cmd);
	}
	
	if (!found) {
		gtk_label_set_text (GTK_LABEL (self->priv->interfaceContent), "");
		gtk_label_set_text (GTK_LABEL (self->priv->networkContent), "");
		gtk_label_set_text (GTK_LABEL (self->priv->ipContent), _("(no IP found)"));
	}
	else
	{
		if (self->priv->dbus_icd2_proxy)
			dbus_g_proxy_begin_call (self->priv->dbus_icd2_proxy, ICD_GET_STATISTICS_REQ, (DBusGProxyCallNotify) personal_ip_address_on_method_call_finished, self, NULL, G_TYPE_INVALID);
	}
	self->priv->isConnected = found;
}

gboolean personal_ip_address_connection_changed (ConIcConnection *connection, ConIcConnectionEvent *event, PersonalIpAddress *self)
{
	//g_warning ("%s", __PRETTY_FUNCTION__);
	personal_ip_address_update_content (self);
	return TRUE;
}

void personal_ip_address_connectOnPress_changed (HildonCheckButton *button, GtkWidget *target)
{
	gtk_widget_set_sensitive (target, hildon_check_button_get_active (button));
}

void personal_ip_address_settings (HDHomePluginItem *hitem, PersonalIpAddress *self)
{
	//g_warning ("personal_ip_address_settings");
	GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Personal IP Address Settings"),
        NULL, 0, _tran("wdgt_bd_save"), GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	GtkSizeGroup *group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));
	
	GtkWidget *connectOnPress = hildon_check_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (connectOnPress), _("Connect to network when pressed"));
	gtk_container_add (GTK_CONTAINER (content_area), connectOnPress);
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (connectOnPress), self->priv->connectOnPress);
	
	GtkWidget *disconnectOnPress = hildon_check_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (disconnectOnPress), _("Disconnect from network when pressed"));
	gtk_container_add (GTK_CONTAINER (content_area), disconnectOnPress);
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (disconnectOnPress), self->priv->disconnectOnPress);
	gtk_widget_set_sensitive (disconnectOnPress, self->priv->connectOnPress);
	g_signal_connect (connectOnPress, "toggled", G_CALLBACK (personal_ip_address_connectOnPress_changed), disconnectOnPress);
		
	GtkWidget *compactMode = hildon_check_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (compactMode), _("Use compact layout"));
	gtk_container_add (GTK_CONTAINER (content_area), compactMode);
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (compactMode), self->priv->compactMode);
	
	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	
	GtkWidget *showInterface = hildon_gtk_radio_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT, NULL);
	gtk_button_set_label (GTK_BUTTON (showInterface), _("Show Network Interface"));
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (showInterface), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), showInterface, TRUE, TRUE, 0);
	
	GtkWidget *showNetwork = hildon_gtk_radio_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT, gtk_radio_button_get_group (GTK_RADIO_BUTTON (showInterface)));
	gtk_button_set_label (GTK_BUTTON (showNetwork), _("Show Network Name"));
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (showNetwork), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), showNetwork, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (content_area), hbox);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (showInterface), self->priv->showInterface);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (showNetwork), !self->priv->showInterface);
	
	gtk_widget_show_all (dialog);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		self->priv->connectOnPress = hildon_check_button_get_active (HILDON_CHECK_BUTTON (connectOnPress));
		self->priv->disconnectOnPress = hildon_check_button_get_active (HILDON_CHECK_BUTTON (disconnectOnPress));
		self->priv->compactMode = hildon_check_button_get_active (HILDON_CHECK_BUTTON (compactMode));
		self->priv->showInterface = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (showInterface));
		
		if (self->priv->compactMode) {
			gtk_widget_hide (self->priv->ipImage);
			if (self->priv->showInterface) {
				gtk_widget_show (self->priv->ipLabel);
				gtk_widget_show (self->priv->interfaceContent);
				gtk_widget_hide (self->priv->networkContent);
				gtk_widget_set_size_request (GTK_WIDGET (self), 315, 40);
				gtk_window_resize (GTK_WINDOW (self), 315, 40);
			} else {
				gtk_widget_hide (self->priv->ipLabel);
				gtk_widget_hide (self->priv->interfaceContent);
				gtk_widget_show (self->priv->networkContent);
				gtk_widget_set_size_request (GTK_WIDGET (self), 352, 40);
				gtk_window_resize (GTK_WINDOW (self), 352, 40);
			}
		} else {
			gtk_widget_hide (self->priv->ipLabel);
			gtk_widget_show (self->priv->ipImage);
			if (self->priv->showInterface) {
				gtk_widget_show (self->priv->interfaceContent);
				gtk_widget_hide (self->priv->networkContent);
				gtk_widget_set_size_request (GTK_WIDGET (self), 315, 56);
				gtk_window_resize (GTK_WINDOW (self), 315, 56);
			} else {
				gtk_widget_hide (self->priv->interfaceContent);
				gtk_widget_show (self->priv->networkContent);
				gtk_widget_set_size_request (GTK_WIDGET (self), 352, 56);
				gtk_window_resize (GTK_WINDOW (self), 352, 56);
			}
		}
		
		personal_ip_address_write_settings (self);
		personal_ip_address_update_content (self);
	}
	gtk_widget_destroy (dialog);
}

static void personal_ip_address_check_desktop (GObject *gobject, GParamSpec *pspec, PersonalIpAddress *self)
{
	//g_warning ("personal_ip_address_check_desktop");
	/*gchar *name = pspec->name;
	gboolean status;
	g_object_get (gobject, name, &status, NULL);
	if (status) {
		personal_ip_address_update_content (self);
		if (self->priv->eventID == 0) {
			self->priv->eventID = g_timeout_add (1*1000*60, (GSourceFunc)personal_ip_address_update_content, self);
		}
	} else if (self->priv->eventID != 0) {
		g_source_remove (self->priv->eventID);
		self->priv->eventID = 0;
	}*/
}

static void personal_ip_address_dispose (GObject *object)
{
	//g_warning ("personal_ip_address_dispose");
	PersonalIpAddress *self = PERSONAL_IP_ADDRESS (object);

	G_OBJECT_CLASS (personal_ip_address_parent_class)->dispose (object);
}

static void personal_ip_address_finalize (GObject *object)
{
	//g_warning ("personal_ip_address_finalize");
	PersonalIpAddress *self = PERSONAL_IP_ADDRESS (object);

	if (self->priv->eventID) {
		g_source_remove (self->priv->eventID);
	}
	g_object_unref (self->priv->connection);

	if (self->priv->dbus_icd2_proxy)
		g_object_unref (self->priv->dbus_icd2_proxy);

	G_OBJECT_CLASS (personal_ip_address_parent_class)->finalize (object);
}

static void personal_ip_address_realize (GtkWidget *widget)
{
	//g_warning ("personal_ip_address_realize");
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	gtk_widget_set_colormap (widget, gdk_screen_get_rgba_colormap (screen));

	gtk_widget_set_app_paintable (widget, TRUE);

	GTK_WIDGET_CLASS (personal_ip_address_parent_class)->realize (widget);
}

static gboolean personal_ip_address_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	//g_warning ("personal_ip_address_expose_event");
	PersonalIpAddress *self = PERSONAL_IP_ADDRESS (widget);
	cairo_t *cr;
	
	cr = gdk_cairo_create(GDK_DRAWABLE (widget->window));

	GdkColor color;
	if (!self->priv->isPressed) {
		gtk_style_lookup_color (gtk_rc_get_style(widget), "DefaultBackgroundColor", &color);
		cairo_set_source_rgba (cr, color.red/65535.0, color.green/65335.0, color.blue/65535.0, 0.75);
	} else {
		gtk_style_lookup_color (gtk_rc_get_style(widget), "SelectionColor", &color);
		cairo_set_source_rgba (cr, color.red/65535.0, color.green/65335.0, color.blue/65535.0, 0.6);
	}
	
	gint width, height, x, y;
	gint radius = 5;
	width = widget->allocation.width;
	height = widget->allocation.height;
	x = widget->allocation.x;
	y = widget->allocation.y;

	if (!self->priv->compactMode) {
		height = 40;
		y += 16;
	}

	cairo_move_to(cr, x + radius, y);
	cairo_line_to(cr, x + width - radius, y);
	cairo_curve_to(cr, x + width - radius, y, x + width, y, x + width,
				y + radius);
	cairo_line_to(cr, x + width, y + height - radius);
	cairo_curve_to(cr, x + width, y + height - radius, x + width,
				y + height, x + width - radius, y + height);
	cairo_line_to(cr, x + radius, y + height);
	cairo_curve_to(cr, x + radius, y + height, x, y + height, x,
				y + height - radius);
	cairo_line_to(cr, x, y + radius);
	cairo_curve_to(cr, x, y + radius, x, y, x + radius, y);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	cairo_fill_preserve(cr);
	
	gtk_style_lookup_color (gtk_rc_get_style(widget), "ActiveTextColor", &color);
	cairo_set_source_rgba (cr, color.red/65535.0, color.green/65335.0, color.blue/65535.0, 0.5);
	cairo_set_line_width (cr, 1);
	cairo_stroke (cr);
	
	cairo_destroy(cr);

	return GTK_WIDGET_CLASS (personal_ip_address_parent_class)->expose_event (widget, event);
}

static void personal_ip_address_class_init (PersonalIpAddressClass *klass)
{
	//g_warning ("personal_ip_address_class_init");
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->dispose = personal_ip_address_dispose;
	object_class->finalize = personal_ip_address_finalize;
	
	widget_class->realize = personal_ip_address_realize;
	widget_class->expose_event = personal_ip_address_expose_event;

	g_type_class_add_private (klass, sizeof (PersonalIpAddressPrivate));
}

static void personal_ip_address_class_finalize (PersonalIpAddressClass *klass G_GNUC_UNUSED)
{
}

static void personal_ip_address_init (PersonalIpAddress *self)
{
	//g_warning ("personal_ip_address_init");
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	self->priv = PERSONAL_IP_ADDRESS_GET_PRIVATE (self);
	self->priv->eventID = 0;
	self->priv->isPressed = FALSE;
	self->priv->isConnected = FALSE;
	
	self->priv->connection = con_ic_connection_new ();
	g_signal_connect (self->priv->connection, "connection-event", G_CALLBACK (personal_ip_address_connection_changed), self);
	g_object_set (self->priv->connection, "automatic-connection-events", TRUE);

	self->priv->dbus_connection = hd_home_plugin_item_get_dbus_g_connection (&self->parent, DBUS_BUS_SYSTEM, NULL);
	if (self->priv->dbus_connection)
		self->priv->dbus_icd2_proxy = dbus_g_proxy_new_for_name (self->priv->dbus_connection, ICD_DBUS_SERVICE, ICD_DBUS_PATH, ICD_DBUS_INTERFACE);
	
	hd_home_plugin_item_set_settings (&self->parent, TRUE);
	g_signal_connect (&self->parent, "show-settings", G_CALLBACK (personal_ip_address_settings), self);

	gtk_window_set_default_size (GTK_WINDOW (self), 352, 56);
	
	self->priv->homeWidget = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (self), self->priv->homeWidget);
	gtk_widget_show (self->priv->homeWidget);

	GdkGeometry hints;
	hints.min_width = 315;
	hints.min_height = 40;
	hints.max_width = 352;
	hints.max_height = 56;
	gtk_window_set_geometry_hints (GTK_WINDOW (self), self->priv->homeWidget, &hints, GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
	
	personal_ip_address_read_settings (self);
	
	if (self->priv->compactMode) {
		if (self->priv->showInterface) {
			gtk_widget_set_size_request (GTK_WIDGET (self), 320, 40);
			gtk_window_resize (GTK_WINDOW (self), 320, 40);
		} else {
			gtk_widget_set_size_request (GTK_WIDGET (self), 352, 40);
			gtk_window_resize (GTK_WINDOW (self), 352, 40);
		}
	} else {
		if (self->priv->showInterface) {
			gtk_widget_set_size_request (GTK_WIDGET (self), 320, 56);
			gtk_window_resize (GTK_WINDOW (self), 320, 56);
		} else {
			gtk_widget_set_size_request (GTK_WIDGET (self), 352, 56);
			gtk_window_resize (GTK_WINDOW (self), 352, 56);
		}			
	}
	
	personal_ip_address_content_create (self);
	personal_ip_address_update_content (self);
	
	g_signal_connect (self, "notify::is-on-current-desktop", G_CALLBACK (personal_ip_address_check_desktop), self);
}

PersonalIpAddress* personal_ip_address_new (void)
{
  return g_object_new (PERSONAL_TYPE_IP_ADDRESS, NULL);
}
