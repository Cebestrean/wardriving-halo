.class public final Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;
.super Ljava/lang/Object;
.source "HaloViewModel.kt"

# interfaces
.implements Landroid/hardware/SensorEventListener;


# annotations
.annotation system Ldalvik/annotation/EnclosingMethod;
    value = Lcom/wifihalo/viewmodel/HaloViewModel;-><init>(Landroid/app/Application;)V
.end annotation

.annotation system Ldalvik/annotation/InnerClass;
    accessFlags = 0x19
    name = null
.end annotation

.annotation runtime Lkotlin/Metadata;
    d1 = {
        "\u0000-\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u0014\n\u0002\u0008\u0003\n\u0002\u0010\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\u0008\u0003\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u0008\n\u0000*\u0001\u0000\u0008\n\u0018\u00002\u00020\u0001J\u0010\u0010\u0006\u001a\u00020\u00072\u0006\u0010\u0008\u001a\u00020\tH\u0016J\u0008\u0010\n\u001a\u00020\u0007H\u0002J\u001a\u0010\u000b\u001a\u00020\u00072\u0008\u0010\u000c\u001a\u0004\u0018\u00010\r2\u0006\u0010\u000e\u001a\u00020\u000fH\u0016R\u000e\u0010\u0002\u001a\u00020\u0003X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0004\u001a\u00020\u0003X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0005\u001a\u00020\u0003X\u0082\u0004\u00a2\u0006\u0002\n\u0000\u00a8\u0006\u0010"
    }
    d2 = {
        "com/wifihalo/viewmodel/HaloViewModel$compassListener$1",
        "Landroid/hardware/SensorEventListener;",
        "rotMatrix",
        "",
        "inclMatrix",
        "orientation",
        "onSensorChanged",
        "",
        "event",
        "Landroid/hardware/SensorEvent;",
        "computeOrientation",
        "onAccuracyChanged",
        "sensor",
        "Landroid/hardware/Sensor;",
        "accuracy",
        "",
        "app_debug"
    }
    k = 0x1
    mv = {
        0x2,
        0x2,
        0x0
    }
    xi = 0x30
.end annotation


# instance fields
.field private final inclMatrix:[F

.field private final orientation:[F

.field private final rotMatrix:[F

.field final synthetic this$0:Lcom/wifihalo/viewmodel/HaloViewModel;


# direct methods
.method constructor <init>(Lcom/wifihalo/viewmodel/HaloViewModel;)V
    .locals 2
    .param p1, "$receiver"    # Lcom/wifihalo/viewmodel/HaloViewModel;

    iput-object p1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    .line 306
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    .line 307
    const/16 v0, 0x9

    new-array v1, v0, [F

    iput-object v1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->rotMatrix:[F

    .line 308
    new-array v0, v0, [F

    iput-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->inclMatrix:[F

    .line 309
    const/4 v0, 0x3

    new-array v0, v0, [F

    iput-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->orientation:[F

    .line 306
    return-void
.end method

.method private final computeOrientation()V
    .locals 4

    .line 333
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getHasGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Z

    move-result v0

    if-eqz v0, :cond_2

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getHasMagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Z

    move-result v0

    if-nez v0, :cond_0

    goto :goto_0

    .line 334
    :cond_0
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->rotMatrix:[F

    iget-object v1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->inclMatrix:[F

    iget-object v2, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v2}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v2

    iget-object v3, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v3}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v3

    invoke-static {v0, v1, v2, v3}, Landroid/hardware/SensorManager;->getRotationMatrix([F[F[F[F)Z

    move-result v0

    if-eqz v0, :cond_1

    .line 335
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->rotMatrix:[F

    iget-object v1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->orientation:[F

    invoke-static {v0, v1}, Landroid/hardware/SensorManager;->getOrientation([F[F)[F

    .line 336
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_compass$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v0

    iget-object v1, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->orientation:[F

    const/4 v2, 0x0

    aget v1, v1, v2

    float-to-double v1, v1

    invoke-static {v1, v2}, Ljava/lang/Math;->toDegrees(D)D

    move-result-wide v1

    double-to-float v1, v1

    const/high16 v2, 0x43b40000    # 360.0f

    add-float/2addr v1, v2

    rem-float/2addr v1, v2

    invoke-static {v1}, Ljava/lang/Float;->valueOf(F)Ljava/lang/Float;

    move-result-object v1

    invoke-interface {v0, v1}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    .line 338
    :cond_1
    return-void

    .line 333
    :cond_2
    :goto_0
    return-void
.end method


# virtual methods
.method public onAccuracyChanged(Landroid/hardware/Sensor;I)V
    .locals 2
    .param p1, "sensor"    # Landroid/hardware/Sensor;
    .param p2, "accuracy"    # I

    .line 341
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_compassAccuracy$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v0

    invoke-static {p2}, Ljava/lang/Integer;->valueOf(I)Ljava/lang/Integer;

    move-result-object v1

    invoke-interface {v0, v1}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    .line 342
    return-void
.end method

.method public onSensorChanged(Landroid/hardware/SensorEvent;)V
    .locals 8
    .param p1, "event"    # Landroid/hardware/SensorEvent;

    const-string v0, "event"

    invoke-static {p1, v0}, Lkotlin/jvm/internal/Intrinsics;->checkNotNullParameter(Ljava/lang/Object;Ljava/lang/String;)V

    .line 312
    iget-object v0, p1, Landroid/hardware/SensorEvent;->sensor:Landroid/hardware/Sensor;

    invoke-virtual {v0}, Landroid/hardware/Sensor;->getType()I

    move-result v0

    const/4 v1, 0x3

    const v2, 0x3e99999a    # 0.3f

    const/4 v3, 0x2

    const/4 v4, 0x1

    const/4 v5, 0x0

    sparse-switch v0, :sswitch_data_0

    goto/16 :goto_2

    .line 314
    :sswitch_0
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->rotMatrix:[F

    iget-object v2, p1, Landroid/hardware/SensorEvent;->values:[F

    invoke-static {v0, v2}, Landroid/hardware/SensorManager;->getRotationMatrixFromVector([F[F)V

    .line 315
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->rotMatrix:[F

    iget-object v2, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->orientation:[F

    invoke-static {v0, v2}, Landroid/hardware/SensorManager;->getOrientation([F[F)[F

    .line 316
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_compass$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v0

    iget-object v2, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->orientation:[F

    aget v2, v2, v5

    float-to-double v2, v2

    invoke-static {v2, v3}, Ljava/lang/Math;->toDegrees(D)D

    move-result-wide v2

    double-to-float v2, v2

    const/high16 v3, 0x43b40000    # 360.0f

    add-float/2addr v2, v3

    rem-float/2addr v2, v3

    invoke-static {v2}, Ljava/lang/Float;->valueOf(F)Ljava/lang/Float;

    move-result-object v2

    invoke-interface {v0, v2}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    .line 317
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$get_compassAccuracy$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Lkotlinx/coroutines/flow/MutableStateFlow;

    move-result-object v0

    invoke-static {v1}, Ljava/lang/Integer;->valueOf(I)Ljava/lang/Integer;

    move-result-object v1

    invoke-interface {v0, v1}, Lkotlinx/coroutines/flow/MutableStateFlow;->setValue(Ljava/lang/Object;)V

    goto/16 :goto_2

    .line 325
    :sswitch_1
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getHasMagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Z

    move-result v0

    if-nez v0, :cond_0

    iget-object v0, p1, Landroid/hardware/SensorEvent;->values:[F

    iget-object v2, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v2}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v2

    invoke-static {v0, v5, v2, v5, v1}, Ljava/lang/System;->arraycopy(Ljava/lang/Object;ILjava/lang/Object;II)V

    goto :goto_0

    .line 326
    :cond_0
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v0

    aget v1, v0, v5

    iget-object v6, p1, Landroid/hardware/SensorEvent;->values:[F

    aget v6, v6, v5

    iget-object v7, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v7}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v7

    aget v7, v7, v5

    sub-float/2addr v6, v7

    mul-float/2addr v6, v2

    add-float/2addr v1, v6

    aput v1, v0, v5

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v0

    aget v1, v0, v4

    iget-object v5, p1, Landroid/hardware/SensorEvent;->values:[F

    aget v5, v5, v4

    iget-object v6, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v6}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v6

    aget v6, v6, v4

    sub-float/2addr v5, v6

    mul-float/2addr v5, v2

    add-float/2addr v1, v5

    aput v1, v0, v4

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v0

    aget v1, v0, v3

    iget-object v5, p1, Landroid/hardware/SensorEvent;->values:[F

    aget v5, v5, v3

    iget-object v6, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v6}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGeomagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v6

    aget v6, v6, v3

    sub-float/2addr v5, v6

    mul-float/2addr v5, v2

    add-float/2addr v1, v5

    aput v1, v0, v3

    .line 327
    :goto_0
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0, v4}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$setHasMagnetic$p(Lcom/wifihalo/viewmodel/HaloViewModel;Z)V

    invoke-direct {p0}, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->computeOrientation()V

    goto :goto_2

    .line 320
    :sswitch_2
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getHasGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)Z

    move-result v0

    if-nez v0, :cond_1

    iget-object v0, p1, Landroid/hardware/SensorEvent;->values:[F

    iget-object v2, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v2}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v2

    invoke-static {v0, v5, v2, v5, v1}, Ljava/lang/System;->arraycopy(Ljava/lang/Object;ILjava/lang/Object;II)V

    goto :goto_1

    .line 321
    :cond_1
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v0

    aget v1, v0, v5

    iget-object v6, p1, Landroid/hardware/SensorEvent;->values:[F

    aget v6, v6, v5

    iget-object v7, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v7}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v7

    aget v7, v7, v5

    sub-float/2addr v6, v7

    mul-float/2addr v6, v2

    add-float/2addr v1, v6

    aput v1, v0, v5

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v0

    aget v1, v0, v4

    iget-object v5, p1, Landroid/hardware/SensorEvent;->values:[F

    aget v5, v5, v4

    iget-object v6, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v6}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v6

    aget v6, v6, v4

    sub-float/2addr v5, v6

    mul-float/2addr v5, v2

    add-float/2addr v1, v5

    aput v1, v0, v4

    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v0

    aget v1, v0, v3

    iget-object v5, p1, Landroid/hardware/SensorEvent;->values:[F

    aget v5, v5, v3

    iget-object v6, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v6}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$getGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;)[F

    move-result-object v6

    aget v6, v6, v3

    sub-float/2addr v5, v6

    mul-float/2addr v5, v2

    add-float/2addr v1, v5

    aput v1, v0, v3

    .line 322
    :goto_1
    iget-object v0, p0, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->this$0:Lcom/wifihalo/viewmodel/HaloViewModel;

    invoke-static {v0, v4}, Lcom/wifihalo/viewmodel/HaloViewModel;->access$setHasGravity$p(Lcom/wifihalo/viewmodel/HaloViewModel;Z)V

    invoke-direct {p0}, Lcom/wifihalo/viewmodel/HaloViewModel$compassListener$1;->computeOrientation()V

    .line 330
    :goto_2
    return-void

    nop

    :sswitch_data_0
    .sparse-switch
        0x1 -> :sswitch_2
        0x2 -> :sswitch_1
        0xb -> :sswitch_0
        0x14 -> :sswitch_0
    .end sparse-switch
.end method
