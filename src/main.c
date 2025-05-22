#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>
#include "v_common.h"
#include "v_log.h"

#define TARGET_UUID "0000xxxx-0000-1000-8000-00805f9b34fb"      // 目的のサービスUUID
#define TARGET_CHAR_UUID "0000yyyy-0000-1000-8000-00805f9b34fb" // 目的のキャラクタリスティックUUID

static void on_characteristic_changed(GDBusConnection *connection,
                                      const gchar *sender_name,
                                      const gchar *object_path,
                                      const gchar *interface_name,
                                      const gchar *signal_name,
                                      GVariant *parameters,
                                      gpointer user_data);

static GDBusConnection *conn = NULL;
static gchar *target_device_path = NULL;

// スキャンの開始
void start_discovery()
{
    g_dbus_connection_call(conn,
                           "org.bluez",
                           "/org/bluez/hci0",
                           "org.bluez.Adapter1",
                           "StartDiscovery",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL, NULL);
}

// スキャンの停止
void stop_discovery()
{
    g_dbus_connection_call(conn,
                           "org.bluez",
                           "/org/bluez/hci0",
                           "org.bluez.Adapter1",
                           "StopDiscovery",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL, NULL);
}

// デバイスの接続
void connect_device(const gchar *device_path)
{
    g_dbus_connection_call(conn,
                           "org.bluez",
                           device_path,
                           "org.bluez.Device1",
                           "Connect",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL, NULL);
}

// ServicesResolved プロパティの変更を監視
void on_properties_changed(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data)
{
    GVariantIter *props;
    const gchar *iface;
    GVariant *value;
    g_variant_get(parameters, "(&sa{sv}as)", &iface, &props, NULL);
    while (g_variant_iter_next(props, "{&sv}", &iface, &value))
    {
        if (g_strcmp0(iface, "ServicesResolved") == 0 && g_variant_get_boolean(value))
        {
            // サービスが解決されたらキャラクタリスティックを探索
            // ここでキャラクタリスティックの探索と通知の有効化を行う
        }
        g_variant_unref(value);
    }
    g_variant_iter_free(props);
}

void enable_notify(GDBusConnection *conn, const char *char_path)
{
    // Notify有効化
    g_dbus_connection_call(
        conn,
        "org.bluez",
        char_path,
        "org.bluez.GattCharacteristic1",
        "StartNotify",
        NULL,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        NULL);

    // PropertiesChangedシグナルにコールバックを登録
    g_dbus_connection_signal_subscribe(
        conn,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        char_path,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_characteristic_changed, // 別途実装済みのコールバック
        NULL,
        NULL);
}
char *find_gatt_service(GDBusConnection *conn, const char *adapter_path, const char *device_path, const char *target_uuid)
{
    GVariant *objects = g_dbus_connection_call_sync(
        conn,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        NULL);
    if (!objects)
        return NULL;

    GVariantIter *iter;
    g_variant_get(objects, "(a{oa{sa{sv}}})", &iter);

    const char *object_path;
    GVariant *interfaces;
    while (g_variant_iter_next(iter, "{&oa{sa{sv}}}", &object_path, &interfaces))
    {
        if (!g_str_has_prefix(object_path, device_path))
        {
            g_variant_unref(interfaces);
            continue;
        }

        GVariantIter iface_iter;
        g_variant_iter_init(&iface_iter, interfaces);
        const char *iface_name;
        GVariant *props;
        while (g_variant_iter_next(&iface_iter, "{&sa{sv}}", &iface_name, &props))
        {
            if (g_strcmp0(iface_name, "org.bluez.GattService1") == 0)
            {
                GVariant *uuid_var = g_variant_lookup_value(props, "UUID", NULL);
                if (uuid_var && g_strcmp0(g_variant_get_string(uuid_var, NULL), target_uuid) == 0)
                {
                    g_variant_unref(uuid_var);
                    g_variant_unref(props);
                    g_variant_unref(interfaces);
                    g_variant_iter_free(iter);
                    g_variant_unref(objects);
                    return g_strdup(object_path); // 呼び出し元で解放必要
                }
                g_variant_unref(uuid_var);
            }
            g_variant_unref(props);
        }
        g_variant_unref(interfaces);
    }

    g_variant_iter_free(iter);
    g_variant_unref(objects);
    return NULL;
}

char *find_gatt_characteristic(GDBusConnection *conn, const char *service_path, const char *target_char_uuid)
{
    GVariant *objects = g_dbus_connection_call_sync(
        conn,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        NULL);
    if (!objects)
        return NULL;

    GVariantIter *iter;
    g_variant_get(objects, "(a{oa{sa{sv}}})", &iter);

    const char *object_path;
    GVariant *interfaces;
    while (g_variant_iter_next(iter, "{&oa{sa{sv}}}", &object_path, &interfaces))
    {
        if (!g_str_has_prefix(object_path, service_path))
        {
            g_variant_unref(interfaces);
            continue;
        }

        GVariantIter iface_iter;
        g_variant_iter_init(&iface_iter, interfaces);
        const char *iface_name;
        GVariant *props;
        while (g_variant_iter_next(&iface_iter, "{&sa{sv}}", &iface_name, &props))
        {
            if (g_strcmp0(iface_name, "org.bluez.GattCharacteristic1") == 0)
            {
                GVariant *uuid_var = g_variant_lookup_value(props, "UUID", NULL);
                if (uuid_var && g_strcmp0(g_variant_get_string(uuid_var, NULL), target_char_uuid) == 0)
                {
                    g_variant_unref(uuid_var);
                    g_variant_unref(props);
                    g_variant_unref(interfaces);
                    g_variant_iter_free(iter);
                    g_variant_unref(objects);
                    return g_strdup(object_path); // 呼び出し元で解放必要
                }
                g_variant_unref(uuid_var);
            }
            g_variant_unref(props);
        }
        g_variant_unref(interfaces);
    }

    g_variant_iter_free(iter);
    g_variant_unref(objects);
    return NULL;
}

static void on_characteristic_changed(GDBusConnection *connection,
                                      const gchar *sender_name,
                                      const gchar *object_path,
                                      const gchar *interface_name,
                                      const gchar *signal_name,
                                      GVariant *parameters,
                                      gpointer user_data)
{
    GVariant *value;
    const gchar *iface;
    GVariantIter *props;
    char buf[MAX_PATH], *p;
    p = buf;

    g_variant_get(parameters, "(&sa{sv}as)", &iface, &props, NULL);
    while (g_variant_iter_next(props, "{&sv}", &iface, &value))
    {
        if (g_strcmp0(iface, "Value") == 0)
        {
            GVariantIter *array;
            guchar byte;
            g_variant_get(value, "ay", &array);
            p = buf;
            p += sprintf(p, "Notify data: ");
            while (g_variant_iter_next(array, "y", &byte))
            {
                p += sprintf(p, "%02x ", byte);
            }
            d("%s", buf);
            g_variant_iter_free(array);
        }
        g_variant_unref(value);
    }
    g_variant_iter_free(props);
}
// InterfacesAdded シグナルのハンドラ
void on_interfaces_added(GDBusConnection *connection,
                         const gchar *sender_name,
                         const gchar *object_path,
                         const gchar *interface_name,
                         const gchar *signal_name,
                         GVariant *parameters,
                         gpointer user_data)
{
    GVariantIter *interfaces;
    const gchar *object;
    g_variant_get(parameters, "(&oa{sa{sv}})", &object, &interfaces);
    d("%s", object);
    if (g_str_has_prefix(object, "/org/bluez/hci0/dev_"))
    {
        // Device1 インターフェースの UUIDs プロパティを取得
        GVariant *result = g_dbus_connection_call_sync(conn,
                                                       "org.bluez",
                                                       object,
                                                       "org.freedesktop.DBus.Properties",
                                                       "Get",
                                                       g_variant_new("(ss)", "org.bluez.Device1", "UUIDs"),
                                                       NULL,
                                                       G_DBUS_CALL_FLAGS_NONE,
                                                       -1,
                                                       NULL,
                                                       NULL);

        if (!result)
            return;
        GVariant *uuid_variant;
        GVariantIter iter;
        const gchar *uuid;
        g_variant_get(result, "(v)", &uuid_variant);
        g_variant_iter_init(&iter, uuid_variant);

        while (g_variant_iter_loop(&iter, "s", &uuid))
        {
            d("  UUID: %s", uuid);
            if (g_strcmp0(uuid, TARGET_UUID) == 0)
            {
                stop_discovery();
                connect_device(object);
                break;
            }
        }
    }
    g_variant_iter_free(interfaces);
}

int main(int argc, char *argv[])
{
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    // InterfacesAdded シグナルの監視
    g_dbus_connection_signal_subscribe(conn,
                                       "org.bluez",
                                       "org.freedesktop.DBus.ObjectManager",
                                       "InterfacesAdded",
                                       NULL,
                                       NULL,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       on_interfaces_added,
                                       NULL,
                                       NULL);

    // PropertiesChanged シグナルの監視
    g_dbus_connection_signal_subscribe(conn,
                                       "org.bluez",
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged",
                                       NULL,
                                       NULL,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       on_properties_changed,
                                       NULL,
                                       NULL);

    start_discovery();
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return 0;
}
