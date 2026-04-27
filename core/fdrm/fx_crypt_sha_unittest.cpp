// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fdrm/fx_crypt_sha.h"

#include <string>
#include <vector>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/span.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAreArray;

namespace {

struct ShaTestData {
  const char* input;
  std::string expected_hex;
  const char* description;
};

std::vector<uint8_t> HexToBytes(const std::string& hex) {
  std::vector<uint8_t> bytes;
  bytes.reserve(hex.size() / 2);
  for (size_t i = 0; i < hex.size(); i += 2) {
    uint8_t byte =
        static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
    bytes.push_back(byte);
  }
  return bytes;
}

}  // namespace

TEST(FXCryptShaTest, Sha1) {
  const std::vector<ShaTestData> kTests = {
      {"", "da39a3ee5e6b4b0d3255bfef95601890afd80709", "Empty"},
      {"a", "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8", "Single char"},
      {" ", "b858cb282617fb0956d960215c8e84d1ccf909c6", "Space"},
      {"abc", "a9993e364706816aba3e25717850c26c9cd0d89d", "Simple string"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "84983e441c3bd26ebaae4aa1f95129e5e54670f1", "Longer string"}};
  for (const auto& test : kTests) {
    const std::string input(test.input);
    DataVector<uint8_t> actual = CryptSha1Generate(
        pdfium::as_bytes(pdfium::span<const char>(input)));
    EXPECT_THAT(actual, ElementsAreArray(HexToBytes(test.expected_hex)))
        << "SHA1 Failed: " << test.description;
  }
}

TEST(FXCryptShaTest, Sha256) {
  const std::vector<ShaTestData> kTests = {
      {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
       "Empty"},
      {"a", "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb",
       "Single char"},
      {" ", "36a9e7f1c95b82ffb99743e0c5c4ce95d83c9a430aac59f84ef3cbfab6145068",
       "Space"},
      {"The quick brown fox jumps over the lazy dog",
       "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
       "Sentence"}};
  for (const auto& test : kTests) {
    const std::string input(test.input);
    DataVector<uint8_t> actual = CryptSha256Generate(
        pdfium::as_bytes(pdfium::span<const char>(input)));
    EXPECT_THAT(actual, ElementsAreArray(HexToBytes(test.expected_hex)))
        << "SHA256 Failed: " << test.description;
  }
}

TEST(FXCryptShaTest, Sha384) {
  const std::vector<ShaTestData> kTests = {
    {"",
     "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b",
     "Empty"},
    {"a",
     "54a59b9f22b0b80880d8427e548b7c23abd873486e1f035dce9cd697e85175033caa88e6d57bc35efae0b5afd3145f31",
     "Single char"},
    {" ",
     "588016eb10045dd85834d67d187d6b97858f38c58c690320c4a64e0c2f92eebd9f1bd74de256e8268815905159449566",
     "Space"},
    {"\t\n\r",
     "a18273bbf2232dd3d0cfab8669e83ebed41575330debd42cbef464892dc29a676f486548174f1f93e508272938ce95f2",
     "Control chars"},
    {"1234567890123456789012345678901234567890",
     "b2501fc3833ae6feba7dc8a973a22d709b7c796ee97cbf66db2c22df873a9fa147b1b630878f771457b7769efd9ffa0d",
     "Numbers long"},
    {"!@#$%^&*()_+-=[]{}|;':\",./<>?",
     "d34f069335f7377f5553b253f728262dd2700ab7e1cdb6a7a9d1eb846ed0acfe335e195148a71741b8ea2aed79390b17",
     "Special chars"},
    {"The quick brown fox jumps over the lazy dog",
     "ca737f1014a48f4c0b6dd43cb177b0afd9e5169367544c494011e3317dbf9a509cb1e5dc1e85a941bbee3d7f2afbc9b1",
     "Standard sentence"},
    {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
     "501e72a20c43fc3f2ec502f9518a7d96e8075ff555a9d9c3bc0d88a271916626669c1388b6eaec0a6c3e0690b7c4b5b9",
     "Alphanumeric"}};

  for (const auto& test : kTests) {
    const std::string input(test.input);
    DataVector<uint8_t> actual = CryptSha384Generate(
        pdfium::as_bytes(pdfium::span<const char>(input)));
    EXPECT_THAT(actual, ElementsAreArray(HexToBytes(test.expected_hex)))
        << "Failed: " << test.description;
  }
}

TEST(FXCryptShaTest, Sha512) {
const std::vector<ShaTestData> kTests = {
    {"",
     "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
     "Empty"},
    {"a",
     "1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f5302860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75",
     "Single char"},
    {" ",
     "f90ddd77e400dfe6a3fcf479b00b1ee29e7015c5bb8cd70f5f15b4886cc339275ff553fc8a053f8ddc7324f45168cffaf81f8c3ac93996f6536eef38e5e40768",
     "Space"},
    {"\t\n\r",
     "59087def905f1db0c3da4805da7541dc300bdb04e5a75d88b9e95c67cf54baaf29bd1fda621c986ba81ebc94051a308bd90ed18753deabe210f5b4aa5a71d170",
     "Control chars"},
    {"1234567890123456789012345678901234567890",
     "3a8529d8f0c7b1ad2fa54c944952829b718d5beb4ff9ba8f4a849e02fe9a272daf59ae3bd06dde6f01df863d87c8ba4ab016ac576b59a19078c26d8dbe63f79e",
     "Numbers long"},
    {"!@#$%^&*()_+-=[]{}|;':\",./<>?",
     "8de14838555200f1eb32cd90395d582cbaab7fcc31badf3eca61bb7ed3ee5c998de3f580da0e03812de186647fb6e5da389f7abd52ee2254ab0ebcf94f856459",
     "Special chars"},
    {"The quick brown fox jumps over the lazy dog",
     "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6",
     "Standard sentence"},
    {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
     "e26b7daef4366128fc7ae3fb75f31789ae03648c61b91192ac6cffb4924a1ce53c0768fe21daab635f5aebaa7fd112b325fd6a32715926c3d73d1ac31e6431a5",
     "Alphanumeric"}};

  for (const auto& test : kTests) {
    const std::string input(test.input);
    DataVector<uint8_t> actual = CryptSha512Generate(
        pdfium::as_bytes(pdfium::span<const char>(input)));
    EXPECT_THAT(actual, ElementsAreArray(HexToBytes(test.expected_hex)))
        << "Failed: " << test.description;
  }
}

TEST(FXCryptShaTest, Sha512BlockBoundary127) {
  const std::string input(127, 'A');
  DataVector<uint8_t> actual =
      CryptSha512Generate(pdfium::as_bytes(pdfium::span<const char>(input)));
  EXPECT_THAT(
      actual,
      ElementsAreArray(HexToBytes(
          "a9b159065acda6e2eee883409f5dc796afae042237ace277514f72152a480465ed44877cc5f8267434ce9187fcdfe8f8a4f69314a60815513a6b6c63287f5e29"
          )));
}

TEST(FXCryptShaTest, Sha512BlockBoundary128) {
  const std::string input(128, 'A');
  DataVector<uint8_t> actual =
      CryptSha512Generate(pdfium::as_bytes(pdfium::span<const char>(input)));
  EXPECT_THAT(
      actual,
      ElementsAreArray(HexToBytes(
          "6486a74d95f54812a76071f6c6344ab6d34df3da685ec70dc78d9c5804b4ee3c449d9e68a6b52491f8275b838c2cd9102c3c223a620bbee2671edbff2611594e")));
}

TEST(FXCryptShaTest, Sha512LargeInput1K) {
  const std::string input(1024, 'z');
  DataVector<uint8_t> actual =
      CryptSha512Generate(pdfium::as_bytes(pdfium::span<const char>(input)));
  EXPECT_THAT(
      actual,
      ElementsAreArray(HexToBytes(
          "1cf081f4e5d7624e3bb8fb06d2e99e88adab3f6ebcd102aaa3fc9d91433aa3e6f64779c891ffac2abee265e05d50e7882aa41a29054c120095c62a3641d71b04")));
}

TEST(FXCryptShaTest, Sha1Streaming) {
  CryptSha1Context ctx;
  CryptSha1Start(&ctx);

  const std::string p1 = "The qui";
  const std::string p2 = "ck brown fox jumps over ";
  const std::string p3 = "the lazy dog";

  CryptSha1Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p1)));
  CryptSha1Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p2)));
  CryptSha1Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p3)));

  uint8_t digest[20];
  CryptSha1Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12")));
}

TEST(FXCryptShaTest, Sha256Streaming) {
  CryptSha2Context ctx;
  CryptSha256Start(&ctx);

  const std::string p1 = "The qui";
  const std::string p2 = "ck brown fox jumps over ";
  const std::string p3 = "the lazy dog";

  CryptSha256Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p1)));
  CryptSha256Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p2)));
  CryptSha256Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p3)));

  uint8_t digest[32];
  CryptSha256Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes(
          "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592")));
}

TEST(FXCryptShaTest, Sha384Streaming) {
  CryptSha2Context ctx;
  CryptSha384Start(&ctx);

  const std::string p1 = "The qui";
  const std::string p2 = "ck brown fox jumps over ";
  const std::string p3 = "the lazy dog";

  CryptSha384Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p1)));
  CryptSha384Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p2)));
  CryptSha384Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p3)));

  uint8_t digest[48];
  CryptSha384Finish(&ctx, digest);
  EXPECT_THAT(digest, ElementsAreArray(HexToBytes(
                          "ca737f1014a48f4c0b6dd43cb177b0afd9e5169367544c494011e3317dbf9a509cb1e5dc1e85a941bbee3d7f2afbc9b1")));
}

TEST(FXCryptShaTest, Sha512Streaming) {
  CryptSha2Context ctx;
  CryptSha512Start(&ctx);

  const std::string p1 = "The qui";
  const std::string p2 = "ck brown fox jumps over ";
  const std::string p3 = "the lazy dog";

  CryptSha512Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p1)));
  CryptSha512Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p2)));
  CryptSha512Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(p3)));

  uint8_t digest[64];
  CryptSha512Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes(
          "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6")));
}

TEST(FXCryptShaTest, Sha1Multilingual) {
  CryptSha1Context ctx;
  CryptSha1Start(&ctx);

  const std::string input = "AI is 爱";
  CryptSha1Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(input)));

  uint8_t digest[20];
  CryptSha1Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes("b60475886fd054ebef2d468c8f6da340f3e913b0")));
}

TEST(FXCryptShaTest, Sha256Multilingual) {
  CryptSha2Context ctx;
  CryptSha256Start(&ctx);

  const std::string input = "AI is 爱";
  CryptSha256Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(input)));

  uint8_t digest[32];
  CryptSha256Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes(
          "528e772b8927444e28fa1ac533a3e7c8563cba1607b3391bb4793fa232d9d8b5")));
}

TEST(FXCryptShaTest, Sha384Multilingual) {
  CryptSha2Context ctx;
  CryptSha384Start(&ctx);

  const std::string input = "AI is 爱";
  CryptSha384Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(input)));

  uint8_t digest[48];
  CryptSha384Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes(
          "8f6102d978bae3e3e87cf5e3ec796c8c218565ef5598a6c2d1414483f96194a51c5bf6d7180ab284632b5072da82e28a")));
}

TEST(FXCryptShaTest, Sha512Multilingual) {
  CryptSha2Context ctx;
  CryptSha512Start(&ctx);

  const std::string input = "AI is 爱";
  CryptSha512Update(&ctx, pdfium::as_bytes(pdfium::span<const char>(input)));

  uint8_t digest[64];
  CryptSha512Finish(&ctx, digest);
  EXPECT_THAT(
      digest,
      ElementsAreArray(HexToBytes(
          "9fd807a40ba510e93123a2e2ad5429621a4e166c1ab863f1a6d1a6f388597b5edd19547268f8f7a9d64eb768ef144e7b0ffe98b11ea7612e682895e24a2259d3")));
}
