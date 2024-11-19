class A:
	@warning_ignore("unused_private_class_variable")
	var __a
	static var __b

	func __c():
		pass
	static func __d():
		pass

class B:
	var a = A.new()
	var b = a.__a
	var c = A.__b

	func foo():
		@warning_ignore("standalone_expression")
		a.__a
		@warning_ignore("standalone_expression")
		A.__b

		a.__c()
		A.__d()

	var d = a.__c()
	var e = A.__d()

class C extends A:
	var a = A.new()
	var b = a.__a
	var c = A.__b

	func foo():
		@warning_ignore("standalone_expression")
		a.__a
		@warning_ignore("standalone_expression")
		A.__b

		a.__c()
		A.__d()

	var d = a.__c()
	var e = A.__d()

func test():
	pass
