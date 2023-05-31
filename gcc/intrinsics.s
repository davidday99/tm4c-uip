.syntax unified
.p2align 2

.global DisableInterrupts
.type   DisableInterrupts,%function
DisableInterrupts:
    CPSID   I
    BX      LR

.global EnableInterrupts
.type   EnableInterrupts,%function
EnableInterrupts:
    CPSIE   I
    BX      LR

# *********** StartCritical ************************
# make a copy of previous I bit, disable interrupts
.global StartCritical
.type   StartCritical,%function
StartCritical:
    MRS     R0, PRIMASK @ save old status
    CPSID   I           @ mask all (except faults)
    BX      LR

# *********** EndCritical ************
# restore I bit to its previous value
.global EndCritical
.type   EndCritical,%function
EndCritical:
    MSR     PRIMASK, R0 @ restore
    BX      LR
    
