The GIA (GPU Image Archive) file format
===================

The goal is to create an image format GPU friendly. This is a hobby experiment.

Features
--------

GIA archives are supposed to be processed once and then distributed. The header
contains useful data that are too often computed on the client (like Android's
Palette API, a phash.org, opaque flag, ...) and a list of data "blocks".

- Blocks (tldr: file composed of skippable blobs, may be in different formats)
    * GIA Is an Archive, not an image format.
    * The archive is streamable, it consist of a header and a list of blocks.
    * Intended to contain the same image using different texture compression
        codecs, and raw pixels data as backup.
    * A decoder may easily ignore blocks in an unsupported/unpreferred format.
    * A server could easily construct a new GIA on the fly by transferring to
        client only blocks in it's preferred format.

- "Native" lossy (tldr: block texture compression, BCn and ASTC)
    * Compressed in a random access format that GPU can easily work with.
    * It reduces GPU memory usage and archive size, but will have
        a small impact on draw performances.

- "Lossless" (tldr: raw pixel data)
    * Raw RGB(A) data (may be compressed using LZ4).
    * Failsafe for systems not supporting other texture compression codecs
        in the archive.

- Mipmaping (tldr: allowing progressive rendering, at size cost)
    * Precomputed mipmaps may be stored along with the source image.
    * Stored from lowest to highest level of detail.
    * Set your max LoD bias, to latest read mipmap for progressive rendering.
    * Client may stop to read when LoD is satisfactory.
    * Some mip levels can be omitted, it will be ok as long as you update your
        LoD bias while the file is loading.

- Stream arrangements (tldr: improve compression, at decode speed cost)
    * De-interleaved texture blocks colors and selector bits, so we have a
        stream of colors and selector bits, instead of having them interleaved
        for each blocks.
    * Per-channel data encoding, this is a technique similar to PNG "sub"
        filtering, we encode the delta of the previous (on the left) pixel,
        instead of specifying the color every time, to increase duplicate rate.
    * Thus techniques require extra steps while decoding, but may not be
        applied on first mip levels, so that you can display something asap.

- Chroma subsampling (tldr: reduce GPU memory usage at cost of draw time)
    * The image is decomposed in two textures using Y+CgCo(A), one grayscale at
        the resolution of the image, and one 2/3 channels texture for the
        chromas(+alpha) at half the resolution of the image.
    * It requires work in fragment shader, but will reduce GPU memory usage
        and archive size.

- Palette (tldr: palette encoded to reduce file size at cost of draw time)
    * The image is splitted in two textures, a one channel texture at full
        resolution, and a 2/3 channels palette (1px wide and max 256px tall).
    * It requires work in fragment shader, but will reduce GPU memory usage
        and archive size.
    * Can be used in combination with chroma-subsampling (on the chromas+alpha).

- Animation (tldr: dumb animation support, this is not a video format)
    * Support for texture arrays, that could be used for animation.
    * Every blocks has extents, saying what are the data bounds, so that we
        don't have to submit all pixel_count every frame.
    * Every blocks also has a possible clear color, so for example if an image
        has some padding (even if not animated), we don't have to encode that
        padding.

File structure
--------------

    Header:
        signature
        image info
        for each texture
            texture info

    Then the blocks, that will be interleaved:
        for each textures
            for each mipmap (from lowest to highest level of detail)
                for each array slice (sorted by index)
                    for each archive formats (no particular order)
                        block info
                        data

Blocks are interleaved, here's a chroma-subsampled and mipmapped image with
a 4 slices animation:

    tex0_mip8_sli0
    tex1_mip8_sli0  Can already display something
    tex0_mip8_sli1
    tex1_mip8_sli1  Can start playing animation
    tex0_mip8_sli2
    tex1_mip8_sli2
    tex0_mip8_sli3
    tex1_mip8_sli3  Can already play the full animation

    tex0_mip7_sli0  Continue reading until client is satisfied by LoD
    ...

See gia.hpp for a detail and extra info.
Integers in the archive are little endian.