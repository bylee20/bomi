# Maintainer: xylosper <darklin20@gmail.com>
# Contributor: willemw <willemw12@gmail.com>

pkgname=cmplayer
pkgver=0.8.17
pkgrel=1
pkgdesc="Powerful and easy-to-use multimedia player"
arch=('i686' 'x86_64')
url="http://$pkgname.github.io"
license=('GPL')
install=$pkgname.install
depends=('qt5-base' 'qt5-declarative' 'qt5-x11extras' 'qt5-quickcontrols' 'icu'
         'libdvdread' 'libdvdnav' 'libcdio-paranoia' 'libcdio'
         'alsa-lib' 'libpulse' 'jack' 'libchardet' 'libbluray'
         'mpg123' 'libva' 'libgl' 'fribidi' 'libass' 'ffmpeg')
makedepends=('mesa' 'gcc' 'pkg-config' 'python')
optdepends=('libaacs: AACS decryption for Blu-ray support'
            'libbdplus: BD+ decryption for Blu-ray support'
            'youtube-dl: streaming website support including YouTube')
source=(https://github.com/xylosper/cmplayer/releases/download/v$pkgver/cmplayer-$pkgver-source.tar.gz)
md5sums=('7158a294a41832198297ba4d95d3ba97')
options=(debug !strip)

build() {
  cd "$srcdir/$pkgname-$pkgver"
  ./configure --prefix=/usr --enable-jack --enable-cdda
  make
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  make DEST_DIR=$pkgdir install
}
