#include "monort_suite.h"

#include "monopp/mono_assembly.h"
#include "monopp/mono_domain.h"
#include "monopp/mono_field_invoker.h"
#include "monopp/mono_internal_call.h"
#include "monopp/mono_method_invoker.h"
#include "monopp/mono_object.h"
#include "monopp/mono_property_invoker.h"
#include "monopp/mono_string.h"
#include "monopp/mono_type.h"

#include "monort/monort.h"
#include <iostream>

struct vec2f
{
	float x;
	float y;
};

namespace mono
{
namespace managed_interface
{
struct vector2f
{
	float x;
	float y;
};
template <>
inline auto converter::convert(const vec2f& v) -> vector2f
{
	return vector2f{v.x, v.y};
}

template <>
inline auto converter::convert(const vector2f& v) -> vec2f
{
	return vec2f{v.x, v.y};
}
} // namespace managed_interface

register_basic_mono_converter_for_pod(vec2f, managed_interface::vector2f);
register_basic_mono_converter_for_wrapper(std::shared_ptr<vec2f>);
} // namespace mono

void MyVec_TestInternalPODCall(const mono::mono_object& this_ptr, const vec2f& value)
{
	mono::ignore(this_ptr, value);
	std::cout << "FROM C++ : Test Vector2f." << std::endl;
}

void MyVec_CreateInternalCtor(const mono::mono_object& this_ptr, float x, float y)
{
	std::cout << "FROM C++ : WrapperVector2f created." << std::endl;
	using vec2f_ptr = std::shared_ptr<vec2f>;
	auto p = std::make_shared<vec2f>();
	p->x = x;
	p->y = y;

	mono::managed_interface::mono_object_wrapper<vec2f_ptr>::create(this_ptr, p);
}

void MyVec_CreateInternalCopyCtor(const mono::mono_object& this_ptr, const std::shared_ptr<vec2f>& rhs)
{
	std::cout << "FROM C++ : WrapperVector2f created." << std::endl;
	using vec2f_ptr = std::shared_ptr<vec2f>;
	auto p = std::make_shared<vec2f>();
	p->x = rhs->x;
	p->y = rhs->y;

	mono::managed_interface::mono_object_wrapper<vec2f_ptr>::create(this_ptr, p);
}

namespace monort
{

void test_suite(test::suite& suite)
{
	mono::mono_domain domain("domain");
	mono::mono_domain::set_current_domain(domain);

	// clang-format off
	suite.it("init monort", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto core_assembly = domain.get_assembly("monort_managed.dll");
			mono::managed_interface::init(core_assembly);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("bind monort", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			mono::add_internal_call("Tests.MonortTest::TestInternalPODCall(Tests.Vector2f)",
									internal_call(MyVec_TestInternalPODCall));
			mono::add_internal_call("Tests.WrapperVector2f::.ctor(single,single)",
									internal_call(MyVec_CreateInternalCtor));
			mono::add_internal_call("Tests.WrapperVector2f::.ctor(Tests.WrapperVector2f)",
									internal_call(MyVec_CreateInternalCopyCtor));
		};

		spec.expect_nothrow(expression);
	});

	suite.it("get monort valid type", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");

			std::cout << type.get_fullname() << std::endl;
			auto fields = type.get_fields();
			auto props = type.get_properties();
			auto methods = type.get_methods();
			for(const auto& field : fields)
			{
				std::cout << field.get_full_declname() << std::endl;
			}
			for(const auto& prop : props)
			{
				std::cout << prop.get_full_declname() << std::endl;
			}
			for(const auto& method : methods)
			{
				std::cout << method.get_full_declname() << std::endl;
			}
		};

		spec.expect_nothrow(expression);
	});

	suite.it("call member method 3", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto obj = type.new_instance();

			auto method_thunk = mono::make_method_invoker<vec2f(vec2f)>(type, "MethodPodAR");
			vec2f p;
			p.x = 12;
			p.y = 15;
			auto result = method_thunk(obj, p);
			spec.expect_equals(result.x, 165.0f);
			spec.expect_equals(result.y, 7.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("call member method 4", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");

			auto obj = type.new_instance();

			using vec2f_ptr = std::shared_ptr<vec2f>;

			auto ptr = std::make_shared<vec2f>();
			ptr->x = 12;
			ptr->y = 15;

			auto method_thunk = mono::make_method_invoker<vec2f_ptr(vec2f_ptr)>(type, "MethodPodARW");
			auto result = method_thunk(obj, ptr);

			spec.expect_not_null(result.get());
			spec.expect_equals(result->x, 55.0f);
			spec.expect_equals(result->y, 66.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("test member POD field", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto field = type.get_field("someFieldPOD");
			auto obj = type.new_instance();
			spec.expect_true(obj.valid());

            auto mutable_field = mono::make_field_invoker<vec2f>(field);
			auto someField = mutable_field.get_value(obj);
			spec.expect_equals(someField.x, 12.0f);
			spec.expect_equals(someField.y, 13.0f);

			vec2f arg = {6.0f, 7.0f};
			mutable_field.set_value(obj, arg);

			someField = mutable_field.get_value(obj);
			spec.expect_equals(someField.x, 6.0f);
			spec.expect_equals(someField.y, 7.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("test member POD property", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto prop = type.get_property("somePropertyPOD");
            auto mutable_prop = mono::make_property_invoker<vec2f>(prop);
            
			auto obj = type.new_instance();
			spec.expect_true(obj.valid());

			auto someProp = mutable_prop.get_value(obj);
			spec.expect_equals(someProp.x, 12.0f);
			spec.expect_equals(someProp.y, 13.0f);

			vec2f arg = {55.0f, 56.0f};
			mutable_prop.set_value(obj, arg);

			someProp = mutable_prop.get_value(obj);
			spec.expect_equals(someProp.x, 55.0f);
			spec.expect_equals(someProp.y, 56.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("test static POD field", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto field = type.get_field("someFieldPODStatic");
            auto mutable_field = mono::make_field_invoker<vec2f>(field);            
			auto someField = mutable_field.get_value();
			spec.expect_equals(someField.x, 12.0f);
			spec.expect_equals(someField.y, 13.0f);

			vec2f arg = {6.0f, 7.0f};
			mutable_field.set_value(arg);

			someField = mutable_field.get_value();
			spec.expect_equals(someField.x, 6.0f);
			spec.expect_equals(someField.y, 7.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("test static POD property", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto prop = type.get_property("somePropertyPODStatic");
            auto mutable_prop = mono::make_property_invoker<vec2f>(prop);            
            
			auto someProp = mutable_prop.get_value();
			spec.expect_equals(someProp.x, 6.0f);
			spec.expect_equals(someProp.y, 7.0f);

			vec2f arg = {55.0f, 56.0f};
			mutable_prop.set_value(arg);

			someProp = mutable_prop.get_value();
			spec.expect_equals(someProp.x, 55.0f);
			spec.expect_equals(someProp.y, 56.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("test static NON-POD field", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto field = type.get_field("someFieldNONPODStatic");
			using vec2f_ptr = std::shared_ptr<vec2f>;
            auto mutable_field = mono::make_field_invoker<vec2f_ptr>(field);
			auto someField = mutable_field.get_value();
			spec.expect_equals(someField->x, 12.0f);
			spec.expect_equals(someField->y, 13.0f);

			vec2f_ptr arg = std::make_shared<vec2f>(vec2f{6.0f, 7.0f});
			mutable_field.set_value(arg);

			someField = mutable_field.get_value();
			spec.expect_equals(someField->x, 6.0f);
			spec.expect_equals(someField->y, 7.0f);
		};

		spec.expect_nothrow(expression);
	});

	suite.it("test static NON-POD property", [&](test::spec& spec)
    {
		auto expression = [&]()
        {
			auto assembly = domain.get_assembly("tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonortTest");
			auto prop = type.get_property("somePropertyNONPODStatic");

			using vec2f_ptr = std::shared_ptr<vec2f>;
            auto mutable_prop = mono::make_property_invoker<vec2f_ptr>(prop);
            
			auto someProp = mutable_prop.get_value();
			spec.expect_equals(someProp->x, 6.0f);
			spec.expect_equals(someProp->y, 7.0f);

			vec2f_ptr arg = std::make_shared<vec2f>(vec2f{55.0f, 56.0f});
			mutable_prop.set_value(arg);

			someProp = mutable_prop.get_value();
			spec.expect_equals(someProp->x, 55.0f);
			spec.expect_equals(someProp->y, 56.0f);
		};

		spec.expect_nothrow(expression);
	});

	// clang-format on
}
}
