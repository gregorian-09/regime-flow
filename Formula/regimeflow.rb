class Regimeflow < Formula
  desc "Regime-adaptive backtesting framework"
  homepage "https://github.com/gregorian-09/regime-flow"
  url "https://github.com/gregorian-09/regime-flow/archive/refs/tags/v1.0.9.tar.gz"
  sha256 "d7903a3f24b0cb299e71b808b923d5041360b0ef264f3edc42cbf642e58480c8"
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
