class Regimeflow < Formula
  desc "Regime-adaptive backtesting framework"
  homepage "https://github.com/gregorian-09/regime-flow"
  url "https://github.com/gregorian-09/regime-flow/archive/refs/tags/v1.0.11.tar.gz"
  sha256 "57964415c9e7d0d75535918f38adfa6fb652ba4956fab808f78543bf3edfe457"
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
