#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "v_log.h"
#include "v_ipc.h"

#if __has_include("v_ext_feature.h")
#include "v_ext_feature.h"
#endif

#define TARGET_UUID "2ae0ffc2-dd80-4bab-a3a2-01c6f6193761"
#define CHARACTERISTIC_UUID "2ae0ffc2-dd81-4bab-a3a2-01c6f6193761"
#define BLUEZ_BUS_NAME "org.bluez"
#define ADAPTER_INTERFACE "org.bluez.Adapter1"
#define BLUEZ_DEVICE "org.bluez.Device1"
#define DBUS_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define OBJECT_MANAGER_IFACE "org.freedesktop.DBus.ObjectManager"
#define DBUS_METHOD_GET "Get"
#define ADAPTER_UUIDS_PROPERTY "UUIDs"
#define DEFAULT_ADAPTER_PATH "/org/bluez/hci0"
#define GATT_SERVICE_IFACE "org.bluez.GattService1"

#define INVALID_SUBSCRIPTION_ID G_MAXUINT

#define SCAN_TIMEOUT_SEC 5

typedef struct str_peripheral
{
    char *device_path;
    char *service_path;
    char *charcteristic_path;
    guint connection_monitor;
    GDBusProxy *adapter_proxy;
} peripheral_t;

/* function */
static void connect(GDBusConnection *conn, const gchar *object);
static GVariant *getprop(GDBusConnection *conn, const gchar *object_path, const char *target, const char *key, const char *format, GVariant **value);

/* variable */
static GMainLoop *main_loop;

peripheral_t _g_target;
peripheral_t *g_target;

static uint32_t g_ble_scan_start_time;

static uint32_t current_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)ts.tv_sec;
}

static void on_properties_changed(GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data)
{
    const gchar *iface;
    GVariant *changed_props;
    GVariantIter iter;
    const gchar *key;
    GVariant *value;

    d("");
    g_variant_get(parameters, "(&s@a{sv}@as)", &iface, &changed_props, NULL);
    if (g_strcmp0(iface, BLUEZ_DEVICE) != 0)
    {
        g_variant_unref(changed_props);
        return;
    }

    g_variant_iter_init(&iter, changed_props);
    while (g_variant_iter_loop(&iter, "{&sv}", &key, &value))
    {
        d("%s", key);
        if (g_strcmp0(key, "Connected") == 0)
        {
            gboolean connected = g_variant_get_boolean(value);
            if (!connected)
            {
                g_print("Device disconnected!\n");
                g_dbus_connection_signal_unsubscribe(connection, g_target->connection_monitor);
                g_target->connection_monitor = INVALID_SUBSCRIPTION_ID;
                connect(connection, g_target->device_path);
            }
        }
    }

    g_variant_unref(changed_props);
}

static gboolean register_connection_monitor(GDBusConnection *conn, GAsyncResult *res, GObject *source_object)
{
    gboolean ret = FALSE;

    GDBusProxy *device_proxy = G_DBUS_PROXY(source_object);
    GError *error = NULL;
    GVariant *result = g_dbus_proxy_call_finish(device_proxy, res, &error);

    if (error)
    {
        g_printerr("Failed to connect: %s\n", error->message);
        g_error_free(error);
        goto error_return;
    }
    g_variant_unref(result);

    const gchar *device_path = g_dbus_proxy_get_object_path(device_proxy);
    g_print("Connected to %s\n", device_path);

    g_target->connection_monitor = g_dbus_connection_signal_subscribe(
        conn,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        device_path,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_properties_changed,
        NULL,
        NULL);
    ret = TRUE;
error_return:
    return ret;
}

static void on_characteristic_changed(GDBusConnection *conn, const gchar *sender_name,
                                      const gchar *object_path, const gchar *interface_name,
                                      const gchar *signal_name, GVariant *parameters, gpointer user_data)
{
    GVariantIter iter;
    const gchar *property_name;
    GVariant *value_variant;

    g_variant_iter_init(&iter, g_variant_get_child_value(parameters, 1));
    while (g_variant_iter_next(&iter, "{sv}", &property_name, &value_variant))
    {
        if (g_strcmp0(property_name, "Value") == 0)
        {
            gsize length;
            const guint8 *value = g_variant_get_fixed_array(value_variant, &length, sizeof(guint8));

            const char *s = dump(value, length);
            gboolean ret = (gboolean)send_ipc(value, (uint8_t)(length & 0xff));
            d("Notification received on %s: %s ret:%d", object_path, s, ret);
            g_variant_unref(value_variant);
        }
    }
}

static void enable_notify(GDBusConnection *conn, const char *characteristic_path)
{
    // characteristic_path = "/org/bluez/hci0/dev_2A_43_70_50_6A_B7/service0028/char0029";

    // code here.
    GError *error = NULL;

    // Notifications を有効化するため、`StartNotify` メソッドを呼び出す
    GVariant *reply = g_dbus_connection_call_sync(
        conn,
        "org.bluez",
        characteristic_path,
        "org.bluez.GattCharacteristic1",
        "StartNotify",
        NULL, // `StartNotify` に引数は不要
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error)
    {
        fprintf(stderr, "Failed to enable notifications on %s: %s\n", characteristic_path, error->message);
        g_error_free(error);
    }
    else
    {
        printf("Notifications enabled on %s\n", characteristic_path);
        g_variant_unref(reply);
    }
    guint subscription_id = g_dbus_connection_signal_subscribe(
        conn, "org.bluez", "org.freedesktop.DBus.Properties",
        "PropertiesChanged", characteristic_path, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_characteristic_changed, NULL, NULL);

    printf("Subscribed to notifications for %s (ID: %u)\n", characteristic_path, subscription_id);
}

static void on_connected(GObject *source_object,
                         GAsyncResult *res,
                         gpointer user_data)
{
    GDBusConnection *conn = (GDBusConnection *)user_data;
    d("conn: %p", conn);

    gboolean ret = register_connection_monitor(conn, res, source_object);
    d("register %d", ret);

    GDBusProxy *device_proxy = G_DBUS_PROXY(source_object);
    const gchar *device_path = g_dbus_proxy_get_object_path(device_proxy);
    d("dev: %s", device_path);
    GError *error = NULL;

    // ObjectManager proxy の作成
    GDBusProxy *obj_manager = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        NULL,
        &error);

    if (error)
    {
        g_printerr("Failed to create ObjectManager proxy: %s\n", error->message);
        g_error_free(error);
        goto error_return;
    }

    // GetManagedObjects 呼び出し
    GVariant *result = g_dbus_proxy_call_sync(
        obj_manager,
        "GetManagedObjects",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error)
    {
        g_printerr("Failed to call GetManagedObjects: %s\n", error->message);
        g_error_free(error);
        g_object_unref(obj_manager);
        goto error_return;
    }

    GVariantIter *iter;
    gchar *object_path;
    GVariant *interfaces;

    g_variant_get(result, "(a{oa{sa{sv}}})", &iter);

    gboolean found = FALSE;
    while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &object_path, &interfaces))
    {
        if (found)
            continue;
        d("%s", object_path);
        // デバイス配下のcharacteristicだけを探す
        if (g_str_has_prefix(object_path, device_path))
        {
            GVariantIter *iface_iter;
            const gchar *iface_name;
            GVariant *props;

            iface_iter = g_variant_iter_new(interfaces);
            while (g_variant_iter_loop(iface_iter, "{&s@a{sv}}", &iface_name, &props))
            {
                if (found)
                    continue;
                if (g_strcmp0(iface_name, "org.bluez.GattCharacteristic1") == 0)
                {
                    // UUIDを取得して表示
                    GVariant *uuid_variant = g_variant_lookup_value(props, "UUID", NULL);
                    if (uuid_variant)
                    {
                        const gchar *uuid = g_variant_get_string(uuid_variant, NULL);
                        g_print("%s u: %s\n", iface_name, uuid);
                        if (strcmp((const char *)uuid, CHARACTERISTIC_UUID) == 0)
                        {
                            found = TRUE;
                            enable_notify(conn, object_path);
                        }
                        else
                        {
                        }

                        g_variant_unref(uuid_variant);
                    }
                }
                // g_variant_unref(props);
            }
            g_variant_iter_free(iface_iter);
        }
        // g_variant_unref(interfaces);
    }

    g_variant_iter_free(iter);
    g_variant_unref(result);
    g_object_unref(obj_manager);
error_return:
    return;
}

static void stop_scan(GDBusConnection *conn)
{
    GError *error = NULL;

    g_ble_scan_start_time = 0;
    g_dbus_proxy_call_sync(g_target->adapter_proxy, "StopDiscovery",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, NULL, NULL);
}

static void connect(GDBusConnection *conn, const gchar *object)
{
    GDBusProxy *device_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        object,
        BLUEZ_DEVICE,
        NULL,
        NULL);

    GError *error = NULL;
    g_dbus_proxy_call(device_proxy, "Connect", NULL,
                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, on_connected, conn);

    g_object_unref(device_proxy);
}

static GVariant *getprop(GDBusConnection *conn, const gchar *object_path, const char *target, const char *key, const char *format, GVariant **value)
{
    GError *error = NULL;
    GVariant *reply = g_dbus_connection_call_sync(
        conn,
        "org.bluez",
        object_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new(format, target, key),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    if (error)
    {
        g_error_free(error);
        goto error_return;
    }

    g_variant_get(reply, "(v)", value);

error_return:
    return reply;
}
static char *get_service_uuid_from_device(GDBusConnection *conn, const gchar *object_path)
{
    GVariant *value;
    char *uuid = NULL;

    GVariant *reply = getprop(conn, object_path, BLUEZ_DEVICE, "UUIDs", "(ss)", &value);
    ERR_RETn(!value || !reply);

    GVariantIter iter;
    g_variant_iter_init(&iter, value);
    while (g_variant_iter_next(&iter, "s", &uuid))
    {
        if (strcmp(TARGET_UUID, uuid) == 0)
        {
            break;
        }
    }

    g_variant_unref(value);
    g_variant_unref(reply);
error_return:
    return uuid;
}
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

    d("");
    g_variant_get(parameters, "(&o@a{sa{sv}})", &object, &interfaces_variant);
    g_variant_iter_init(&iter, interfaces_variant);
    while (g_variant_iter_loop(&iter, "{&s@a{sv}}", &interface_name, &properties))
    {
        if (g_strcmp0(interface_name, BLUEZ_DEVICE) != 0)
            continue;

        d("");
        char *uuid = get_service_uuid_from_device(conn, object);
        if (!uuid)
            continue;
        d("uuid: %s", uuid);
    }
    g_variant_unref(interfaces_variant);
}

static char *introspect(GDBusConnection *conn, char *object_path)
{
    GError *error = NULL;
    gchar *xml_data = NULL;
    GVariant *r = g_dbus_connection_call_sync(
        conn,
        "org.bluez",
        object_path,
        "org.freedesktop.DBus.Introspectable",
        "Introspect",
        NULL,
        G_VARIANT_TYPE("(s)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error)
    {
        fprintf(stderr, "Error: %s\n", error->message);
        g_error_free(error);
        goto error_return;
    }
    g_variant_get(r, "(s)", &xml_data);
error_return:
    g_variant_unref(r);
    return xml_data;
}
static char *get_service_uuid(GDBusConnection *conn, char *object_path)
{
    GError *error = NULL;
    char *uuid = NULL;
    GVariantIter iter;
    gchar *key;
    GVariant *value;
    GVariant *r = g_dbus_connection_call_sync(
        conn,
        "org.bluez",
        object_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.GattService1", "UUID"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error)
    {
        g_error_free(error);
        goto error_return;
    }

    g_variant_get(r, "(v)", &value);
    uuid = (char *)g_variant_print(value, TRUE);
    g_variant_unref(value);
    g_variant_unref(r);
error_return:
    return uuid;
}

static gboolean found_known_device(GDBusConnection *conn)
{
    GError *error = NULL;
    gboolean found = FALSE;
    gchar *object_path = NULL;
    gchar *service_path = NULL;
    gchar *xml = introspect(conn, DEFAULT_ADAPTER_PATH);
    ERR_RETn(!xml);

    // d("xml: %s", xml);

    xmlDocPtr doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
    ERR_RETn(!doc);

    object_path = malloc(MAX_PATH);
    service_path = malloc(MAX_PATH);
    ERR_RETn(!object_path || !service_path);
    xmlNodePtr root = xmlDocGetRootElement(doc);
    for (xmlNodePtr node = root->children; node; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"node") != 0)
            continue;

        xmlChar *name = xmlGetProp(node, (const xmlChar *)"name");
        if (!name)
            continue;

        sprintf(object_path, DEFAULT_ADAPTER_PATH "/%s", name);
        xmlFree(name);

        char *uuid = get_service_uuid_from_device(conn, object_path);
        if (uuid)
        {
            d("uuid: %s", uuid);
            if (strcmp(uuid, TARGET_UUID) == 0)
            {
                found = TRUE;
                g_target->device_path = object_path;
                break;
            }
            else
            {
                g_free(uuid);
            }
        }

        gchar *device = introspect(conn, object_path);
        xmlDocPtr devdoc = xmlReadMemory(device, strlen(device), "noname.xml", NULL, 0);
        if (!devdoc)
        {
            g_free(device);
            continue;
        }

        xmlNodePtr devroot = xmlDocGetRootElement(devdoc);
        for (xmlNodePtr sn = devroot->children; sn; sn = sn->next)
        {
            if (sn->type != XML_ELEMENT_NODE || xmlStrcmp(sn->name, (const xmlChar *)"node") != 0)
                continue;
            xmlChar *service_name = xmlGetProp(sn, (const xmlChar *)"name");
            if (!service_name)
                continue;

            sprintf(service_path, "%s/%s", object_path, service_name);
            char *uuid = get_service_uuid(conn, service_path);
            if (strcmp(uuid, TARGET_UUID) == 0)
            {
                found = TRUE;
                g_target->service_path = service_path;
                g_target->device_path = object_path;
                xmlFree(service_name);
                break;
            }
            g_free(uuid);
            xmlFree(service_name);
        }
        g_free(device);
        if (found)
        {
            d("");
            break;
        }
    }
    xmlFreeDoc(doc);

    if (!found && service_path)
        free(service_path);
error_return:
    if (xml)
        g_free(xml);
    if (!found && object_path)
        free(object_path);
    return found;
}

static void start_scan(GDBusConnection *conn)
{
    GError *error = NULL;
    g_dbus_proxy_call_sync(g_target->adapter_proxy, "StartDiscovery", NULL,
                           G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error)
    {
        g_printerr("Failed to start discovery: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_ble_scan_start_time = current_time();

    d("");
}

static void register_scan_callback(GDBusConnection *conn)
{
    // Signal subscription
    guint signal_id = g_dbus_connection_signal_subscribe(
        conn,
        "org.bluez",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesAdded",
        NULL,
        BLUEZ_DEVICE,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_interface_added,
        NULL,
        NULL);
}

static void connection_info_init(GDBusConnection *conn)
{
    GError *error = NULL;
    g_target = &_g_target;
    g_target->connection_monitor = INVALID_SUBSCRIPTION_ID;
    g_target->adapter_proxy = g_dbus_proxy_new_sync(
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
        g_target = NULL;
        g_error_free(error);
    }
}

gboolean periodic_task(gpointer user_data)
{
    GDBusConnection *conn = (GDBusConnection *)user_data;

#ifdef __V_EXT_FEATURE_H__
    uint8_t process_info[sizeof(v_notify_data_t)];
#else
    uint8_t process_info[3];
#endif

    if (g_ble_scan_start_time)
    {
        uint32_t cur = current_time();
        if ((cur - g_ble_scan_start_time) > SCAN_TIMEOUT_SEC)
        {
            stop_scan(conn);
            d("### ERROR: foot switch not found");
            g_main_loop_quit(main_loop);
        }
        else
        {
            if (found_known_device(conn))
            {
                connect(conn, g_target->device_path);
                stop_scan(conn);
            }
        }
    }

    uint8_t connected = (g_target->connection_monitor != INVALID_SUBSCRIPTION_ID);
#ifdef __V_EXT_FEATURE_H__
    v_notify_data_t *p = (v_notify_data_t *)process_info;
    p->kind = V_IPC_STATUS_NOTIFY;
    p->bt.running = true;
    p->bt.connected = connected;
#else
    process_info[0] = 0x10;
    process_info[1] = true;
    process_info[2] = connected;
#endif
    send_ipc(process_info, sizeof(process_info));
    return TRUE;
}

int main(void)
{
    GDBusConnection *conn;
    GError *error = NULL;

    v_log_init();

    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!conn)
    {
        g_printerr("Failed to connect to system bus: %s\n", error->message);
        g_error_free(error);
        goto error_return;
    }

    connection_info_init(conn);

    if (found_known_device(conn))
    {
        connect(conn, g_target->device_path);
    }
    else
    {
        start_scan(conn);
        register_scan_callback(conn);
    }

    main_loop = g_main_loop_new(NULL, FALSE);
    g_timeout_add(1000, periodic_task, conn);
    g_main_loop_run(main_loop);

error_return:
    if (g_target && g_target->connection_monitor != INVALID_SUBSCRIPTION_ID)
        g_dbus_connection_signal_unsubscribe(conn, g_target->connection_monitor);

    if (g_target)
        g_object_unref(g_target->adapter_proxy);
    g_object_unref(conn);

    return 0;
}
