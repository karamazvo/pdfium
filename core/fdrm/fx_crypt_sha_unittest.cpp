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
      {" ", "b858cb3a0314415456488660b133100650d32042", "Space"},
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
      {" ", "36a9e7f1c95b82ffb99743e0c5c4ce95d83c9a430aac59f84ef3cbf914a30617",
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
     "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274ede76f65fbd51ad2f14898b95bbac",
     "Empty"},
    {"a",
     "1af17e73721d62010675840679c02081d6d132646d84033f677d2426989601d0d9d48b48873f4e9f73a388f8c4749f99",
     "Single char"},
    {" ",
     "277e9262f6b86e06b99f36592233f268b81204859a1d13f5d6f519532885966373f7c1975e523f858276f57e62a87313",
     "Space"},
    {"\t\n\r",
     "a18273bb0f22323d08cfab8669e83ebee4157533ed0d4be0f464892dd0acfe5f227b61386129841f3e30f143717a61f2",
     "Control chars"},
    {"1234567890123456789012345678901234567890",
     "b2501fc3833ae6feba7dc8a973a22da2709b7c796ee97cbf66db2c22df873a9fa1a1a1b24e6274741f22678663806212",
     "Numbers long"},
    {"!@#$%^&*()_+-=[]{}|;':\",./<>?",
     "d34f069335f7377f5553b253f728262d027039cd84acd1eb846ed0acfe725832a829e2f9d4827d6d5ca09c853549646b5a",
     "Special chars"},
    {"The quick brown fox jumps over the lazy dog",
     "ca737f1014a51440673d0ecc76ad85930918744e1d13f9f2d9f291399fd686414995f3334208d960756910609b55231a",
     "Standard sentence"},
    {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
     "501e72a20c43fc3f2ec502f9518a7d96e8075ff555a9d9c3bc0d88a2719166264d26210f8812df08e9206d203f381a81",
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
          "578794697f90f6c2f901198547206121401340a6b73f27f8830154625b596956277b"
          "213b38466e0689b910408502390a18764047a83f9478f7e2792823631f45")));
}

TEST(FXCryptShaTest, Sha512BlockBoundary128) {
  const std::string input(128, 'B');
  DataVector<uint8_t> actual =
      CryptSha512Generate(pdfium::as_bytes(pdfium::span<const char>(input)));
  EXPECT_THAT(
      actual,
      ElementsAreArray(HexToBytes(
          "f34a8190772c9a3d4638706d87f54c93540d99042b5c5e8846937e2832049d5c4149"
          "e9177a5e8062947a118815147575b5b035a9d6092d6408226064f895c275")));
}

TEST(FXCryptShaTest, Sha512LargeInput1K) {
  const std::string input(1024, 'z');
  DataVector<uint8_t> actual =
      CryptSha512Generate(pdfium::as_bytes(pdfium::span<const char>(input)));
  EXPECT_THAT(
      actual,
      ElementsAreArray(HexToBytes(
          "0283f3e1f0e49f6f6959550e56e01297f6424386e8111979b00693a7434932029707"
          "26d42047f6d2f314f364373406368d0d40232491a677353f47285a216828")));
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
                          "ca737f1014a51440673d0ecc76ad85930918744e1d13f9f2d9f2"
                          "91399fd686414995f3334208d960756910609b55231a")));
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
          "07e547d9586f6a73f73fbac0435ed76951218fb40883c11031034c72834bca0ca865"
          "a7f95029a3939677ed2279d63d3fb67f3ba3e71d34346c769493ee2584d3")));
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
      ElementsAreArray(HexToBytes("9819717769975239a52864606709970923053702")));
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
          "29399834e569976378e918501004128509867990141680193155160824141639")));
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
          "62590747444144417102434078516001099688320498146740889212001552601724"
          "6253457002010471207907572620")));
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
          "05814522851446702636502264626107380928929731422709605700813919656402"
          "003923363385150917208826500595304323287612711075677508681024")));
}
