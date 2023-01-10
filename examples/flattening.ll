; ModuleID = 'examples/flattening.c'
source_filename = "examples/flattening.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@.str = private unnamed_addr constant [8 x i8] c"n = %d\0A\00", align 1
@.str.1 = private unnamed_addr constant [22 x i8] c"n is smaller than 10\0A\00", align 1
@.str.2 = private unnamed_addr constant [18 x i8] c"n is equal to 10\0A\00", align 1
@.str.3 = private unnamed_addr constant [21 x i8] c"n is bigger than 10\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i64, align 8
  %3 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %4 = call i32 (ptr, ...) @time(ptr noundef %2)
  call void @srand(i32 noundef %4) #3
  %5 = call i32 @rand() #3
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %6)
  %8 = load i32, ptr %3, align 4
  %9 = icmp slt i32 %8, 10
  br i1 %9, label %10, label %12

10:                                               ; preds = %0
  %11 = call i32 (ptr, ...) @printf(ptr noundef @.str.1)
  br label %20

12:                                               ; preds = %0
  %13 = load i32, ptr %3, align 4
  %14 = icmp eq i32 %13, 10
  br i1 %14, label %15, label %17

15:                                               ; preds = %12
  %16 = call i32 (ptr, ...) @printf(ptr noundef @.str.2)
  br label %19

17:                                               ; preds = %12
  %18 = call i32 (ptr, ...) @printf(ptr noundef @.str.3)
  br label %19

19:                                               ; preds = %17, %15
  br label %20

20:                                               ; preds = %19, %10
  ret i32 0
}

; Function Attrs: nounwind
declare dso_local void @srand(i32 noundef) #1

declare dso_local i32 @time(...) #2

; Function Attrs: nounwind
declare dso_local i32 @rand() #1

declare dso_local i32 @printf(ptr noundef, ...) #2

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 2}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 15.0.6 (Fedora 15.0.6-2.fc37)"}
