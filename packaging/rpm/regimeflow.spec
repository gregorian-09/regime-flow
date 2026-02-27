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
%cmake -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_PYTHON_BINDINGS=OFF
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_bindir}/*
%{_libdir}/*
%{_includedir}/regimeflow
%{_libdir}/cmake/RegimeFlow

%changelog
* Thu Feb 26 2026 RegimeFlow Team <team@regimeflow.io> - 1.0.1-1
- Initial package scaffold
