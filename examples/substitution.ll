declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32)

define i32 @add() {
  %a = add i32 1, 2

  %b = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 15)
  %c = extractvalue { i32, i1 } %b, 0

  ret i32 %c
}

define i32 @sub() {
  %a = call i32 @add()
  %b = sub i32 %a, 1

  ret i32 %b
}