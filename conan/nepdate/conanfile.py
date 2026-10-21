import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy


class NepdateConan(ConanFile):
    name = "nepdate"
    version = "0.1.0-beta.1"
    license = "MIT"
    description = "Bikram Sambat (Nepali calendar) for C++20: BS to AD and back, arithmetic, fiscal year, ranges, formatting and parsing. Header-only, no dependencies."
    homepage = "https://github.com/sanamhub/nepdate-cpp"
    topics = ("nepali", "calendar", "bikram-sambat", "date")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    def layout(self):
        cmake_layout(self)

    def package_id(self):
        self.info.clear()

    def source(self):
        # The CI packages job builds the recipe from the working tree. A Conan Center recipe
        # downloads the release archive here (conandata.yml with url and sha256), added by the
        # maintainer after the tag exists (ADR-0005).
        local = os.environ.get("NEPDATE_CPP_SOURCE")
        if local:
            copy(self, pattern="*", src=local, dst=self.source_folder)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["NEPDATE_BUILD_PATRO"] = "OFF"
        tc.variables["NEPDATE_BUILD_TESTS"] = "OFF"
        tc.variables["NEPDATE_BUILD_TOOLS"] = "OFF"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "nepdate")
        self.cpp_info.set_property("cmake_target_name", "nepdate::nepdate")
        self.cpp_info.libdirs = []
        self.cpp_info.bindirs = []
