class A:
	@warning_ignore("unused_protected_class_variable")
	var _a
	static var _b

	func _c():
		pass
	static func _d():
		pass

class B:
	var a = A.new()
	var b = a._a
	var c = A._b

	func foo():
		@warning_ignore("standalone_expression")
		a._a
		@warning_ignore("standalone_expression")
		A._b

		a._c()
		A._d()

	var d = a._c()
	var e = A._d()

class C extends A:
	var a = A.new()
	var b = a._a
	var c = A._b

	func foo():
		@warning_ignore("standalone_expression")
		a._a
		@warning_ignore("standalone_expression")
		A._b

		a._c()
		A._d()

	var d = a._c()
	var e = A._d()

func test():
	pass
