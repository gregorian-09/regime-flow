class Regimeflow < Formula
  desc "Regime-adaptive backtesting framework"
  homepage "https://github.com/gregorian-09/regime-flow"
  url "https://github.com/gregorian-09/regime-flow/archive/refs/tags/v1.0.1.tar.gz"
  sha256 "666c64d2e75059d25306c2ccd8237692dfe88ff964577aa04cb0095230bae725"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "protobuf"
  depends_on "openssl@3"

  def install
    args = std_cmake_args
    args << "-DBUILD_TESTS=OFF"
    args << "-DBUILD_BENCHMARKS=OFF"
    args << "-DBUILD_PYTHON_BINDINGS=OFF"

    system "cmake", "-S", ".", "-B", "build", *args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    system "#{bin}/regimeflow_alpaca_fetch", "--help"
  end
end
