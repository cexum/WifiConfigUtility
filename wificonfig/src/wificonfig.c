/*
 * wificonfig.c
 * 
 * A GTK app for setting up wifi SSID and WPA_PASSCODE.
 * 
 * Copyright 2018  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <stdlib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

GtkWidget *gentry_ssid;
GtkWidget *gentry_wpa_passkey;
GtkWidget *glabel_instruction;
gint gtimer;
// reads text entries into buffers
// writes those buffers to /etc/wpa_supplicant/wpa_supplicant.conf
// re toggles wlan0

// notify user that app is attempting a connection to SSID
void update_instructions_attempt_connection() 
{
	puts("update_instructions_attempt_connection");
	char temp[32] = {0};
	sprintf(temp, "Connecting to SSID.");
	gtk_label_set_text(GTK_LABEL(glabel_instruction), temp);	
	//g_source_remove(gtimer);
}

// post strings to the GUI
void update_instructions(char *input)
{
	puts("update_instructions");
	char output[256] = {0};
	sprintf(output, "%s", input);
	gtk_label_set_text(GTK_LABEL(glabel_instruction), output);	
}

// saves input from form to /etc/wpa_supplicant/wpa_supplicant.conf
int append_input_to_wpa_supplicant() 
{
	puts("append_input_to_wpa_supplicant");
	
	//declare
	FILE *fp = NULL;	
	char *path = "/etc/wpa_supplicant/wpa_supplicant.conf";
	char temp_ssid[128] = {0};
	char temp_wpa_psk[128] = {0};	
	char temp_wpa_supplicant_append_buffer[256] = {0};
	
	// read text fields into temp buffer
	sprintf(temp_ssid, "%s", gtk_entry_get_text(GTK_ENTRY(gentry_ssid)));
	sprintf(temp_wpa_psk, "%s", gtk_entry_get_text(GTK_ENTRY(gentry_wpa_passkey)));
	printf("temp_ssid: %s\n", temp_ssid);
	printf("temp_ssid: %s\n", temp_wpa_psk);
	
	// write user input to /etc/wpa_supplicant/wpa_supplicant.conf	
	if ((fp = fopen(path, "a")) == NULL) // check if file handle has null reference
	{
		puts("problem opening file!"); 
		return -1; 
	}
		
	sprintf(temp_wpa_supplicant_append_buffer, "\nnetwork={\n\tssid=\"%s\"\n\tpsk=\"%s\"\n}", temp_ssid, temp_wpa_psk); // populate outbound buffer
	puts(temp_wpa_supplicant_append_buffer);	
	fputs(temp_wpa_supplicant_append_buffer, fp); // write out buffer
	fclose(fp);	// destroy file handle
	
	return 0;
}

int toggle_wlan0()
{
	puts("toggle_wlan0");
	system("sudo wpa_cli -i wlan0 reconfigure");	
	
	return 0;		
}	

void get_ip_addr_wlan0(char *input) 
{
	puts("get_ip_addr_wlan0");
	int fd;
	struct ifreq ifr;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);	
	ifr.ifr_addr.sa_family = AF_INET; // get IPv4 IP	
	
	strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1); // parse out ip attached to wlan0
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);	
	sprintf(input, "wlan0 = %s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));	
}

int determine_if_wlan0_connected()
{
	puts("determine_if_wlan0_connected");
	
	int i = 0;
	while (i < 70) // allow time for connection
	{	
	usleep(1000 * 100);
	i++;
	}
	
	char temp_ip_addr[256] = {0};
	get_ip_addr_wlan0(temp_ip_addr);
	update_instructions(temp_ip_addr);
	return 0;
}

int button_connect_clicked_cb() 
{	
	puts("button_connect_clicked_cb");
	
	append_input_to_wpa_supplicant();
	// re-toggle wlan0
	//g_idle_add((GSourceFunc)update_instructions_attempt_connection, NULL);
	
	g_idle_add_full(G_PRIORITY_HIGH, (GSourceFunc)update_instructions_attempt_connection, NULL, NULL);
	//gtimer = g_timeout_add(1000, (GSourceFunc)update_instructions_attempt_connection, NULL);
	toggle_wlan0();
	// tell the user the IP if successful 
	g_idle_add((GSourceFunc)determine_if_wlan0_connected, NULL);
	
	return 0;
}

// destructor
void on_window_main_destroy()
{
	puts("on_window_main_destroy");
    gtk_main_quit(); // le destroy
}
// calls the destructor
void button_exit_clicked_cb() 
{
	puts("button_exit_clicked_cb");
	on_window_main_destroy();
}

int main(int argc, char *argv[])
{
    GtkBuilder      *builder; 
    GtkWidget       *window;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "glade/window_main_wificonfig.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main_wifi_config"));
    gentry_ssid = GTK_WIDGET(gtk_builder_get_object(builder, "entry_SSID"));
    gentry_wpa_passkey = GTK_WIDGET(gtk_builder_get_object(builder, "entry_WPA_PASSKEY"));
    glabel_instruction = GTK_WIDGET(gtk_builder_get_object(builder, "label_instruction"));
    
    gtk_builder_connect_signals(builder, NULL);

    g_object_unref(builder);

    gtk_widget_show(window);
    update_instructions("Please supply your router's SSID and WPA passcode, then press Connect."); 
    gtk_main();

    return 0;
}
