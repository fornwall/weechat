%define name weechat
%define version 0.1.6
%define release 1

Name:      %{name}
Summary:   portable, fast, light and extensible IRC client
Version:   %{version}
Release:   %{release}
Source:    http://weechat.flashtux.org/download/%{name}-%{version}.tar.gz
URL:       http://weechat.flashtux.org
Group:     Networking/IRC
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
Requires:  perl, python, gnutls
License:   GPL
Distribution: Any RPM based distro
Vendor:    FlashCode <flashcode@flashtux.org>

%description
WeeChat (Wee Enhanced Environment for Chat) is a portable, fast, light and
extensible IRC client. Everything can be done with a keyboard.
It is customizable and extensible with scripts.

%prep
rm -rf $RPM_BUILD_ROOT 
%setup

%build
./configure --prefix=/usr --enable-perl --enable-python
make

%install
%makeinstall
mkdir -p $RPM_BUILD_ROOT%{_libdir}/weechat/plugins/
mv $RPM_BUILD_ROOT%{_libdir}/lib* $RPM_BUILD_ROOT%{_libdir}/weechat/plugins/

%find_lang %name

%clean
rm -rf $RPM_BUILD_ROOT 

%files -f %{name}.lang
%defattr(-,root,root,0755) 
%doc AUTHORS BUGS ChangeLog COPYING FAQ FAQ.fr INSTALL NEWS README TODO
%{_mandir}/man1/weechat-curses.1*
%{_bindir}/weechat-curses
%{_libdir}/weechat/plugins/*
%{_infodir}/weechat_doc_en.info*
%{_infodir}/weechat_doc_es.info*
%{_infodir}/weechat_doc_fr.info*
%{_infodir}/weechat_doc_pt.info*

%changelog
* Fri Nov 11 2005 FlashCode <flashcode@flashtux.org> 0.1.6-1
- Released version 0.1.6
* Sat Sep 24 2005 FlashCode <flashcode@flashtux.org> 0.1.5-1
- Released version 0.1.5
* Sat Jul 30 2005 FlashCode <flashcode@flashtux.org> 0.1.4-1
- Released version 0.1.4
* Sat Jul 02 2005 FlashCode <flashcode@flashtux.org> 0.1.3-1
- Released version 0.1.3
* Sat May 21 2005 FlashCode <flashcode@flashtux.org> 0.1.2-1
- Released version 0.1.2
* Sat Mar 20 2005 FlashCode <flashcode@flashtux.org> 0.1.1-1
- Released version 0.1.1
* Sat Feb 12 2005 FlashCode <flashcode@flashtux.org> 0.1.0-1
- Released version 0.1.0
* Sat Jan 01 2005 FlashCode <flashcode@flashtux.org> 0.0.9-1
- Released version 0.0.9
* Sat Oct 30 2004 FlashCode <flashcode@flashtux.org> 0.0.8-1
- Released version 0.0.8
* Sat Aug 08 2004 FlashCode <flashcode@flashtux.org> 0.0.7-1
- Released version 0.0.7
* Sat Jun 05 2004 FlashCode <flashcode@flashtux.org> 0.0.6-1
- Released version 0.0.6
* Thu Feb 02 2004 FlashCode <flashcode@flashtux.org> 0.0.5-1
- Released version 0.0.5
* Thu Jan 01 2004 FlashCode <flashcode@flashtux.org> 0.0.4-1
- Released version 0.0.4
* Mon Nov 03 2003 FlashCode <flashcode@flashtux.org> 0.0.3-1
- Released version 0.0.3
* Sun Oct 05 2003 FlashCode <flashcode@flashtux.org> 0.0.2-1
- Released version 0.0.2
* Sat Sep 27 2003 FlashCode <flashcode@flashtux.org> 0.0.1-1
- Released version 0.0.1
