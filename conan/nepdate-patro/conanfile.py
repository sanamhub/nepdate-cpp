import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.build import check_min_cppstd
from conan.tools.files import copy, get


class NepdatePatroConan(ConanFile):
    name = "nepdate-patro"
    version = "0.1.0-beta.1"
    license = "MIT"
    description = "The patro (Nepali almanac) companion to nepdate: tithi, public holidays and events, BS 2077 to 2084."
    homepage = "https://github.com/sanamhub/nepdate-cpp"
    topics = ("nepali", "calendar", "patro", "tithi")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    no_copy_source = True

    def requirements(self):
        # patro.hpp includes nepdate headers, so consumers need them too. Conan 2 already passes
        # a header-library's headers on; saying so here keeps that true if nepdate's package
        # type ever changes. The test package links only nepdate::patro to prove it.
        self.requires("nepdate/0.1.0-beta.1", transitive_headers=True)

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, 20)

    def source(self):
        # The CI packages job builds the recipe from the working tree; anyone else gets the release
        # archive named in conandata.yml.
        local = os.environ.get("NEPDATE_CPP_SOURCE")
        if local:
            copy(self, pattern="*", src=local, dst=self.source_folder)
        else:
            get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["NEPDATE_BUILD_TESTS"] = "OFF"
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder="patro")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "nepdate-patro")
        self.cpp_info.set_property("cmake_target_name", "nepdate::patro")
        self.cpp_info.libs = ["nepdate_patro"]
        self.cpp_info.requires = ["nepdate::nepdate"]
