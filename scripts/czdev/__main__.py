"""czdev CLI entry point — Python wrapper replacing the Rust binary for publish/unpublish/auth/bump."""

import argparse
import sys


def main():
    parser = argparse.ArgumentParser(
        prog="czdev",
        description="CardputerZero developer CLI. Build, publish and manage apps.",
    )
    subparsers = parser.add_subparsers(dest="command")

    # login
    subparsers.add_parser("login", help="Authenticate with GitHub (device flow).")

    # logout
    subparsers.add_parser("logout", help="Remove stored GitHub credentials.")

    # bump
    bump_parser = subparsers.add_parser("bump", help="Show next version (patch bump) for a package.")
    bump_parser.add_argument("--deb", default=None, help="Path to .deb file. If omitted, searches ./build/*.deb")

    # publish
    pub_parser = subparsers.add_parser("publish", help="Publish a .deb package to the CardputerZero app store.")
    pub_parser.add_argument("--deb", default=None, help="Path to .deb file. If omitted, searches ./build/*.deb")

    # unpublish
    unpub_parser = subparsers.add_parser("unpublish", help="Create a PR to remove a published package.")
    unpub_parser.add_argument("package", help="Package name to remove")
    unpub_parser.add_argument("--version", required=True, help="Version to remove")
    unpub_parser.add_argument("--arch", default="arm64", help="Architecture (default: arm64)")

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(0)

    if args.command == "login":
        from .auth import login
        login()
    elif args.command == "logout":
        from .auth import logout
        logout()
    elif args.command == "bump":
        from .bump import run
        run(deb=args.deb)
    elif args.command == "publish":
        from .publish import run
        run(deb=args.deb)
    elif args.command == "unpublish":
        from .unpublish import run
        run(package=args.package, version=args.version, arch=args.arch)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
