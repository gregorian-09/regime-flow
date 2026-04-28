class Regimeflow < Formula
  desc "Regime-adaptive backtesting framework"
  homepage "https://github.com/gregorian-09/regime-flow"
  url "https://github.com/gregorian-09/regime-flow/archive/refs/tags/v1.0.10.tar.gz"
  sha256 "b5cb75976e3b2ef41905b2f4002e61045d10a28686e8c173d3cd09837beb31a4"
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
