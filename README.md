```
busctl --system introspect org.bluez /org/bluez/hci0/dev_16_CB_19_94_E9_72 
NAME                                TYPE      SIGNATURE RESULT/VALUE                             FLAGS
org.bluez.Device1                   interface -         -                                        -
.CancelPairing                      method    -         -                                        -
.Connect                            method    -         -                                        -
.ConnectProfile                     method    s         -                                        -
.Disconnect                         method    -         -                                        -
.DisconnectProfile                  method    s         -                                        -
.Pair                               method    -         -                                        -
.Adapter                            property  o         "/org/bluez/hci0"                        emits-change
.Address                            property  s         "16:CB:19:94:E9:72"                      emits-change
.AddressType                        property  s         "public"                                 emits-change
.AdvertisingData                    property  a{yv}     -                                        emits-change
.AdvertisingFlags                   property  ay        1 6                                      emits-change
.Alias                              property  s         "16-CB-19-94-E9-72"                      emits-change w>
.Appearance                         property  q         -                                        emits-change
.Blocked                            property  b         false                                    emits-change w>
.Bonded                             property  b         false                                    emits-change
.Class                              property  u         -                                        emits-change
.Connected                          property  b         false                                    emits-change
.Icon                               property  s         -                                        emits-change
.LegacyPairing                      property  b         false                                    emits-change
.ManufacturerData                   property  a{qv}     1 101 ay 3 1 201 5                       emits-change
.Modalias                           property  s         -                                        emits-change
.Name                               property  s         -                                        emits-change
.Paired                             property  b         false                                    emits-change
.RSSI                               property  n         -                                        emits-change
.ServiceData                        property  a{sv}     1 "0000fdf7-0000-1000-8000-00805f9b34fb… emits-change
.ServicesResolved                   property  b         false                                    emits-change
.Sets                               property  a{oa{sv}} -                                        emits-change
.Trusted                            property  b         false                                    emits-change w>
.TxPower                            property  n         -                                        emits-change
.UUIDs                              property  as        2 "0000fe78-0000-1000-8000-00805f9b34fb… emits-change
.WakeAllowed                        property  b         -                                        emits-change w>
org.freedesktop.DBus.Introspectable interface -         -                                        -
.Introspect                         method    -         s                                        -
org.freedesktop.DBus.Properties     interface -         -                                        -
.Get                                method    ss        v                                        -
.GetAll                             method    s         a{sv}                                    -
.Set                                method    ssv       -                                        -
.PropertiesChanged                  signal    sa{sv}as  -                                        -
```
