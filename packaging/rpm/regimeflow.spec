Name:           regimeflow
Version:        1.0.1
Release:        1%{?dist}
Summary:        Regime-adaptive backtesting framework

License:        MIT
URL:            https://github.com/gregorian-09/regime-flow
Source0:        %{url}/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  cmake, gcc-c++, make
Requires:       libstdc++, libgcc

%description
RegimeFlow is a quantitative trading platform that combines regime detection, backtesting, and live execution.

%prep
%autosetup -n regime-flow-%{version}

%build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
  -DREGIMEFLOW_FETCH_DEPS=ON \
  -DBUILD_TESTS=OFF \
  -DBUILD_BENCHMARKS=OFF \
  -DBUILD_PYTHON_BINDINGS=OFF \
  -DENABLE_IBAPI=OFF \
  -DENABLE_ZMQ=OFF \
  -DENABLE_REDIS=OFF \
  -DENABLE_KAFKA=OFF \
  -DENABLE_POSTGRES=OFF \
  -DENABLE_CURL=OFF \
  -DENABLE_OPENSSL=OFF
cmake --build build --parallel %{?_smp_build_ncpus}

%install
rm -rf %{buildroot}
DESTDIR=%{buildroot} cmake --install build

%files
%license LICENSE
%{_libdir}/*.a
%{_includedir}/regimeflow
%{_libdir}/cmake/RegimeFlow

%changelog
* Thu Feb 26 2026 RegimeFlow Team <team@regimeflow.io> - 1.0.1-1
- Initial package scaffold
