The GIA (GPU Image Archive) file format
===================

This project is a hobby experiment.

The goal is to create a lossy image format GPU friendly, minimizing the time
required for decode, and be able to display the image ASAP.

Features
--------

GIA archives are supposed to be processed once and then distributed. The header
contains useful data that are too often computed on the client, like a palette
(as in Android's Palette API), a phash.org, opaque flag, ...

- Blocks (tldr: file composed of skippable blobs, may be in different formats)
    * GIA Is an Archive, not an image format.
    * The archive is streamable, it consist of a header and a list of blocks.
    * Intended to contain the same image using different texture compression
        algorithm, and raw pixels data as backup.
    * Blocks data (independently of it's format) may be optionally compressed
        using LZ4.
    * A decoder may easily ignore blocks in an unsupported/unpreferred format.
    * A server could easily construct a new GIA on the fly by transferring to
        client only blocks in it's preferred format.

- "Native" lossy (tldr: reduce GPU memory usage by storing compressed mipmaps)
    * Same formats as defined in the Vulkan specs (BCn, ETC, ETC2/EAC, ASTC).
    * Compressed in a random access format that GPU can easily work with.
    * It reduces GPU memory usage and archive size, but will have
        a small impact on draw performances.
    * If not compressed by LZ4, blocks' data could be written directly to GPU
        mapped memory. No copy buffer, no decode.

- "Lossless" (tldr: raw data compressed)
    * Raw RGB(A) data (may be compressed using LZ4).
    * FIXME PNG-like filtering to optimize LZ4?

- Mipmaping (tldr: allowing progressive rendering, at size cost)
    * Precomputed mipmaps may be stored along with the source image.
    * Stored from lowest to highest level of detail.
    * Set your max LoD bias, to latest read mipmap for progressive rendering.
    * Client may stop to read when LoD is satisfactory.
    * Some mip levels can be omitted, it will be ok as long as you keep track
        of your LoD bias while the file is loading.

- Chroma subsampling (tldr: reduce GPU memory usage at cost of draw time)
    * The image is decomposed in two textures using Y+CgCo(A), one grayscale at
        the resolution of the image, and one two channels texture for the
        chromas+alpha at half the resolution of the image.
    * It requires work in fragment shader, but will reduce GPU memory usage
        and archive size.
    * If present, the alpha channel is stored with the chromas, at half the
        resolution.

- Palette (tldr: palette encoded to reduce file size at cost of draw time)
    * The image is splitted in two textures, a one channel texture at full
        resolution, and a 2/3 channels palette (1px wide and max 1024px tall),
        to encode the main texture and reduce it's size.
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
-----------

    header:
        signature
        image info
        for each texture
            texture info
    for each textures
        for each mipmap (from lowest to highest level of detail)
            for each array slice (sorted by index)
                for each archive formats (no particular order)
                    block header
                    data

Blocks are interleaved, here's a chroma-subsampled and mipmapped image with
a 4 slices animation:

    tex0_mip0_sli0_s3tc
    tex1_mip0_sli0_s3tc // Could already display something
    tex0_mip0_sli1_s3tc
    tex1_mip0_sli1_s3tc // Could start playing animation
    tex0_mip0_sli2_s3tc
    tex1_mip0_sli2_s3tc
    tex0_mip0_sli3_s3tc
    tex1_mip0_sli3_s3tc // Can already play the full animation

    tex0_mip1_sli0_s3tc // Continue reading to increase LoD
    ...

Integers in the archive are little endian.
See gia.hpp for a detail of the extra info.
