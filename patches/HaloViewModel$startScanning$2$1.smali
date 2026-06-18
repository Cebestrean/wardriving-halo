.class final Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;
.super Ljava/lang/Object;
.source "HaloViewModel.kt"

# interfaces
.implements Lkotlinx/coroutines/flow/FlowCollector;


# annotations
.annotation system Ldalvik/annotation/EnclosingMethod;
    value = Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2;->invokeSuspend(Ljava/lang/Object;)Ljava/lang/Object;
.end annotation

.annotation system Ldalvik/annotation/InnerClass;
    accessFlags = 0x18
    name = null
.end annotation

.annotation system Ldalvik/annotation/Signature;
    value = {
        "<T:",
        "Ljava/lang/Object;",
        ">",
        "Ljava/lang/Object;",
        "Lkotlinx/coroutines/flow/FlowCollector;"
    }
.end annotation

.annotation runtime Lkotlin/Metadata;
    k = 0x3
    mv = {
        0x2,
        0x2,
        0x0
    }
    xi = 0x30
.end annotation


# instance fields
.field final synthetic this$0:Lcom/wifihalo/viewmodel/HaloViewModel;


# direct methods
.method constructor <init>(Lcom/wifihalo/viewmodel/HaloViewModel;)V
    .locals 0

    iput-object p1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method


# virtual methods
.method public final emit(Landroid/location/Location;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;
    .locals 8
    .param p1, "loc"    # Landroid/location/Location;
    .param p2, "$completion"    # Lkotlin/coroutines/Continuation;
    .annotation system Ldalvik/annotation/Signature;
        value = {
            "(",
            "Landroid/location/Location;",
            "Lkotlin/coroutines/Continuation<",
            "-",
            "Lkotlin/Unit;",
            ">;)",
            "Ljava/lang/Object;"
        }
    .end annotation

    .line 879
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_currentLocation$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v0

    invoke-interface {v0, p1}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    # GPS bearing fallback: update compass from GPS when no magnetometer
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getHasMagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Z

    move-result v0

    if-nez v0, :gps_bearing_skip

    invoke-virtual {p1}, Landroid/location/Location;->hasBearing()Z

    move-result v0

    if-eqz v0, :gps_bearing_skip

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_compass$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v0

    invoke-virtual {p1}, Landroid/location/Location;->getBearing()F

    move-result v1

    invoke-static {v1}, Ljava/lang/Float;->valueOf(F)Ljava/lang/Float;

    move-result-object v1

    invoke-interface {v0, v1}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    :gps_bearing_skip

    .line 880
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getWifiScanner$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lcom/wifihalo/scanner/WifiScanner;

    move-result-object v0

    invoke-virtual {v0, p1}, Lcom/wifihalo/scanner/WifiScanner;->setLocation(Landroid/location/Location;)V

    .line 881
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getBluetoothScanner$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lcom/wifihalo/scanner/BluetoothScanner;

    move-result-object v0

    invoke-virtual {v0, p1}, Lcom/wifihalo/scanner/BluetoothScanner;->setLocation(Landroid/location/Location;)V

    .line 882
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getCellularScanner$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lcom/wifihalo/scanner/CellularScanner;

    move-result-object v0

    invoke-virtual {v0, p1}, Lcom/wifihalo/scanner/CellularScanner;->setLocation(Landroid/location/Location;)V

    .line 883
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getSigIntClient$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lcom/wifihalo/scanner/SigIntClient;

    move-result-object v0

    invoke-virtual {v0, p1}, Lcom/wifihalo/scanner/SigIntClient;->setLocation(Landroid/location/Location;)V

    .line 884
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getWalkie$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lcom/wifihalo/comms/WalkieTalkie;

    move-result-object v0

    invoke-virtual {p1}, Landroid/location/Location;->getLatitude()D

    move-result-wide v1

    invoke-virtual {p1}, Landroid/location/Location;->getLongitude()D

    move-result-wide v3

    invoke-virtual {v0, v1, v2, v3, v4}, Lcom/wifihalo/comms/WalkieTalkie;->setLocation(DD)V

    .line 885
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getLastTrackPoint$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Landroid/location/Location;

    move-result-object v0

    .line 886
    .local v0, "prev":Landroid/location/Location;
    if-eqz v0, :cond_0

    .line 887
    invoke-virtual {v0, p1}, Landroid/location/Location;->distanceTo(Landroid/location/Location;)F

    move-result v1

    float-to-double v1, v1

    .line 888
    .local v1, "d":D
    const-wide/high16 v3, 0x3ff0000000000000L    # 1.0

    cmpl-double v3, v1, v3

    if-lez v3, :cond_0

    iget-object v3, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v3}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_distanceMeters$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v3

    invoke-interface {v3}, Lkotlinx/coroutines/flow/MutableStateFlow;->getValue()Ljava/lang/Object;

    move-result-object v4

    check-cast v4, Ljava/lang/Number;

    invoke-virtual {v4}, Ljava/lang/Number;->doubleValue()D

    move-result-wide v4

    add-double/2addr v4, v1

    invoke-static {v4, v5}, Lkotlin/coroutines/jvm/internal/Boxing;->boxDouble(D)Ljava/lang/Double;

    move-result-object v4

    invoke-interface {v3, v4}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    .line 890
    .end local v1    # "d":D
    :cond_0
    iget-object v1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v1, p1}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$setLastTrackPoint$p(Lcom/wifihalo/viewmodel/HaloViewModel;Landroid/location/Location;)V

    .line 891
    iget-object v1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    check-cast v1, Landroidx/lifecycle/ViewModel;

    invoke-static {v1}, Landroidx/lifecycle/ViewModelKt;->getViewModelScope(Landroidx/lifecycle/ViewModel;)Lkotlinx/coroutines/CoroutineScope;

    move-result-object v2

    invoke-static {}, Lkotlinx/coroutines/Dispatchers;->getIO()Lkotlinx/coroutines/CoroutineDispatcher;

    move-result-object v1

    move-object v3, v1

    check-cast v3, Lkotlin/coroutines/CoroutineContext;

    new-instance v1, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1$1;

    iget-object v4, p0, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    const/4 v5, 0x0

    invoke-direct {v1, v4, p1, v5}, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1$1;-><init>(Lcom/wifihalo/viewmodel/HaloViewModel;Landroid/location/Location;Lkotlin/coroutines/Continuation;)V

    move-object v5, v1

    check-cast v5, Lkotlin/jvm/functions/Function2;

    const/4 v6, 0x2

    const/4 v7, 0x0

    const/4 v4, 0x0

    invoke-static/range {v2 .. v7}, Lkotlinx/coroutines/BuildersKt;->launch$default(Lkotlinx/coroutines/CoroutineScope;Lkotlin/coroutines/CoroutineContext;Lkotlinx/coroutines/CoroutineStart;Lkotlin/jvm/functions/Function2;ILjava/lang/Object;)Lkotlinx/coroutines/Job;

    .line 901
    sget-object v1, Lkotlin/Unit;->INSTANCE:Lkotlin/Unit;

    return-object v1
.end method

.method public bridge synthetic emit(Ljava/lang/Object;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;
    .locals 1
    .param p1, "value"    # Ljava/lang/Object;
    .param p2, "$completion"    # Lkotlin/coroutines/Continuation;

    .line 878
    move-object v0, p1

    check-cast v0, Landroid/location/Location;

    invoke-virtual {p0, v0, p2}, Lcom/wifihalo/viewmodel/HaloViewModel$startScanning$2$1;->emit(Landroid/location/Location;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;

    move-result-object v0

    return-object v0
.end method
