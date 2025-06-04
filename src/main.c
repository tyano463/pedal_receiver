#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "v_log.h"

#define TARGET_UUID "2ae0ffc2-dd80-4bab-a3a2-01c6f6193761"

static GMainLoop *main_loop;

static void on_interface_added(GDBusConnection *conn,
                               const gchar *sender_name,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *signal_name,
                               GVariant *parameters,
                               gpointer user_data)
{
    GVariantIter *interfaces;
    const gchar *object;
    const gchar *interface;
    GVariantIter iter;
    GVariant *properties;
    GVariant *interfaces_variant;

    g_variant_get(parameters, "(&o@a{sa{sv}})", &object, &interfaces_variant);
    g_variant_iter_init(&iter, interfaces_variant);
    while (g_variant_iter_loop(&iter, "{&s@a{sv}}", &interface_name, &properties))
    {
        if (g_strcmp0(interface_name, "org.bluez.Device1") != 0)
            continue;

        d("name: %s", interface_name);
        gchar *uuid;
        GVariant *uuids = g_variant_lookup_value(properties, "UUIDs", G_VARIANT_TYPE("as"));
        if (!uuids)
            continue;

        GVariantIter iter;
        g_variant_iter_init(&iter, uuids);
        while (g_variant_iter_loop(&iter, "s", &uuid))
        {
            if (g_ascii_strcasecmp(uuid, TARGET_UUID) == 0)
            {
                g_print("Found device %s with target UUID!\n", object);
                GDBusProxy *device_proxy = g_dbus_proxy_new_sync(
                    conn,
                    G_DBUS_PROXY_FLAGS_NONE,
                    NULL,
                    "org.bluez",
                    object,
                    "org.bluez.Device1",
                    NULL,
                    NULL);

                GError *error = NULL;
                g_dbus_proxy_call_sync(device_proxy, "Connect", NULL,
                                       G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
                if (error)
                {
                    g_printerr("Failed to connect: %s\n", error->message);
                    g_error_free(error);
                }
                else
                {
                    g_print("Connected to device.\n");
                    g_main_loop_quit(main_loop); // 成功したら抜ける
                }

                g_object_unref(device_proxy);
            }
        }
        g_variant_unref(uuids);
    }
    g_variant_unref(interfaces_variant);
}

int main()
{
    GDBusConnection *conn;
    GError *error = NULL;

    v_log_init();

    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!conn)
    {
        g_printerr("Failed to connect to system bus: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    // スキャン開始
    GDBusProxy *adapter_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        NULL,
        &error);

    if (error)
    {
        g_printerr("Failed to create adapter proxy: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    g_dbus_proxy_call_sync(adapter_proxy, "StartDiscovery", NULL,
                           G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    d("");
    if (error)
    {
        g_printerr("Failed to start discovery: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    g_print("Started BLE scan...\n");

    // Signal subscription
    guint signal_id = g_dbus_connection_signal_subscribe(
        conn,
        "org.bluez",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesAdded",
        NULL,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_interface_added,
        NULL,
        NULL);

    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    g_dbus_connection_signal_unsubscribe(conn, signal_id);
    g_object_unref(adapter_proxy);
    g_object_unref(conn);

    return 0;
}
