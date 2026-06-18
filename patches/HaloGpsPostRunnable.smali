.class final Lcom/wifihalo/viewmodel/HaloGpsPostRunnable;
.super Ljava/lang/Object;
.implements Ljava/lang/Runnable;

# Holds a snapshot of the Location and the ViewModel (for gnssStatus).
# Instantiated and started on a new Thread each time the phone gets a GPS fix.
# All network I/O is inside a catch-all so a disconnected DICE never crashes the app.

.field private final loc:Landroid/location/Location;
.field private final vm:Lcom/wifihalo/viewmodel/HaloViewModel;


.method public constructor <init>(Landroid/location/Location;Lcom/wifihalo/viewmodel/HaloViewModel;)V
    .locals 0

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    iput-object p1, p0, Lcom/wifihalo/viewmodel/HaloGpsPostRunnable;->loc:Landroid/location/Location;

    iput-object p2, p0, Lcom/wifihalo/viewmodel/HaloGpsPostRunnable;->vm:Lcom/wifihalo/viewmodel/HaloViewModel;

    return-void
.end method


.method public run()V
    .locals 14

    :try_start_0

    # ── gather location fields ──────────────────────────────────────────────

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloGpsPostRunnable;->loc:Landroid/location/Location;

    invoke-virtual {v0}, Landroid/location/Location;->getLatitude()D
    move-result-wide v1          # v1-v2 = lat (double)

    invoke-virtual {v0}, Landroid/location/Location;->getLongitude()D
    move-result-wide v3          # v3-v4 = lon (double)

    invoke-virtual {v0}, Landroid/location/Location;->getAccuracy()F
    move-result v5               # v5 = acc (float)

    invoke-virtual {v0}, Landroid/location/Location;->getAltitude()D
    move-result-wide v6          # v6-v7 = alt (double)
    double-to-float v6, v6       # v6 = alt (float); v7 now free

    invoke-virtual {v0}, Landroid/location/Location;->getSpeed()F
    move-result v7               # v7 = spd (float)

    invoke-virtual {v0}, Landroid/location/Location;->getBearing()F
    move-result v8               # v8 = brg (float)

    # ── get satellite count from gnssStatus ─────────────────────────────────

    iget-object v9, p0, Lcom/wifihalo/viewmodel/HaloGpsPostRunnable;->vm:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v9}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_gnssStatus$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v9

    invoke-interface {v9}, Lkotlinx/coroutines/flow/MutableStateFlow;->getValue()Ljava/lang/Object;

    move-result-object v9        # v9 = GnssStatusData or null

    const/4 v10, 0x0             # default sats = 0

    if-eqz v9, :no_gnss

    check-cast v9, Lcom/wifihalo/data/GnssStatusData;

    invoke-virtual {v9}, Lcom/wifihalo/data/GnssStatusData;->getTotal()I

    move-result v10              # v10 = total satellite count

    :no_gnss

    # ── build JSON body ─────────────────────────────────────────────────────

    new-instance v9, Ljava/lang/StringBuilder;
    invoke-direct {v9}, Ljava/lang/StringBuilder;-><init>()V

    const-string v11, "{\"lat\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v1, v2}, Ljava/lang/StringBuilder;->append(D)Ljava/lang/StringBuilder;

    const-string v11, ",\"lon\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v3, v4}, Ljava/lang/StringBuilder;->append(D)Ljava/lang/StringBuilder;

    const-string v11, ",\"acc\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v5}, Ljava/lang/StringBuilder;->append(F)Ljava/lang/StringBuilder;

    const-string v11, ",\"alt\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v6}, Ljava/lang/StringBuilder;->append(F)Ljava/lang/StringBuilder;

    const-string v11, ",\"spd\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v7}, Ljava/lang/StringBuilder;->append(F)Ljava/lang/StringBuilder;

    const-string v11, ",\"brg\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v8}, Ljava/lang/StringBuilder;->append(F)Ljava/lang/StringBuilder;

    const-string v11, ",\"sats\":"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9, v10}, Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;

    const-string v11, "}"
    invoke-virtual {v9, v11}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v9}, Ljava/lang/StringBuilder;->toString()Ljava/lang/String;
    move-result-object v9        # v9 = JSON string

    # ── open HTTP connection ────────────────────────────────────────────────

    new-instance v10, Ljava/net/URL;
    const-string v11, "http://192.168.4.1/gps"
    invoke-direct {v10, v11}, Ljava/net/URL;-><init>(Ljava/lang/String;)V

    invoke-virtual {v10}, Ljava/net/URL;->openConnection()Ljava/net/URLConnection;
    move-result-object v10
    check-cast v10, Ljava/net/HttpURLConnection;

    const-string v11, "POST"
    invoke-virtual {v10, v11}, Ljava/net/HttpURLConnection;->setRequestMethod(Ljava/lang/String;)V

    const-string v11, "Content-Type"
    const-string v12, "application/json; charset=utf-8"
    invoke-virtual {v10, v11, v12}, Ljava/net/HttpURLConnection;->setRequestProperty(Ljava/lang/String;Ljava/lang/String;)V

    const/4 v11, 0x1
    invoke-virtual {v10, v11}, Ljava/net/HttpURLConnection;->setDoOutput(Z)V

    # 2000ms timeouts — enough for local AP, won't hang if DICE is off
    const/16 v11, 0x7d0
    invoke-virtual {v10, v11}, Ljava/net/HttpURLConnection;->setConnectTimeout(I)V
    invoke-virtual {v10, v11}, Ljava/net/HttpURLConnection;->setReadTimeout(I)V

    # ── write body ──────────────────────────────────────────────────────────

    invoke-virtual {v10}, Ljava/net/HttpURLConnection;->getOutputStream()Ljava/io/OutputStream;
    move-result-object v11       # v11 = OutputStream

    const-string v12, "UTF-8"
    invoke-virtual {v9, v12}, Ljava/lang/String;->getBytes(Ljava/lang/String;)[B
    move-result-object v12       # v12 = byte[]

    invoke-virtual {v11, v12}, Ljava/io/OutputStream;->write([B)V
    invoke-virtual {v11}, Ljava/io/OutputStream;->flush()V
    invoke-virtual {v11}, Ljava/io/OutputStream;->close()V

    # Calling getResponseCode() actually sends the request and reads the status
    invoke-virtual {v10}, Ljava/net/HttpURLConnection;->getResponseCode()I

    invoke-virtual {v10}, Ljava/net/HttpURLConnection;->disconnect()V

    :try_end_0
    .catch Ljava/lang/Exception; {:try_start_0 .. :try_end_0} :catch_0

    return-void

    :catch_0
    move-exception v0
    return-void
.end method
