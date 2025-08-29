#!/usr/bin/env python3
"""
Continuous Issue Detection and Tracking System
定期的な課題検出・追跡システム
"""

import os
import re
import json
import subprocess
from datetime import datetime
from pathlib import Path

class ContinuousIssueTracker:
    def __init__(self, repo_path: str):
        self.repo_path = Path(repo_path)
        self.issue_history = []
        self.tracking_file = self.repo_path / "docs" / "issue_tracking.json"
        
    def load_history(self):
        """Load previous issue tracking history"""
        if self.tracking_file.exists():
            with open(self.tracking_file, 'r') as f:
                return json.load(f)
        return {"scans": [], "resolved_issues": []}
    
    def save_history(self, history):
        """Save issue tracking history"""
        self.tracking_file.parent.mkdir(exist_ok=True)
        with open(self.tracking_file, 'w') as f:
            json.dump(history, f, indent=2, ensure_ascii=False)
    
    def run_build_analysis(self):
        """Run build and capture warnings/errors"""
        build_dir = self.repo_path / "build"
        issues = []
        
        try:
            # Clean build to capture all warnings
            if build_dir.exists():
                subprocess.run(["rm", "-rf", str(build_dir)], check=True)
            
            build_dir.mkdir(exist_ok=True)
            
            # Run cmake
            cmake_result = subprocess.run(
                ["cmake", ".."], 
                cwd=build_dir, 
                capture_output=True, 
                text=True
            )
            
            # Run make and capture warnings
            make_result = subprocess.run(
                ["make"], 
                cwd=build_dir, 
                capture_output=True, 
                text=True
            )
            
            # Parse warnings from make output
            warnings = self.parse_build_warnings(make_result.stderr)
            for warning in warnings:
                issues.append({
                    "type": "build_warning",
                    "severity": "warning",
                    "file": warning.get("file", "unknown"),
                    "line": warning.get("line", 0),
                    "message": warning.get("message", ""),
                    "detected_at": datetime.now().isoformat()
                })
                
        except subprocess.CalledProcessError as e:
            issues.append({
                "type": "build_error",
                "severity": "error",
                "message": f"Build failed: {e}",
                "detected_at": datetime.now().isoformat()
            })
        
        return issues
    
    def parse_build_warnings(self, stderr_output):
        """Parse warnings from compiler output"""
        warnings = []
        warning_pattern = r'(.+):(\d+):\d+: warning: (.+) \[(.+)\]'
        
        for line in stderr_output.split('\n'):
            match = re.match(warning_pattern, line)
            if match:
                warnings.append({
                    "file": match.group(1),
                    "line": int(match.group(2)),
                    "message": match.group(3),
                    "category": match.group(4)
                })
        
        return warnings
    
    def check_code_quality_issues(self):
        """Check for various code quality issues"""
        issues = []
        
        # Check for TODO/FIXME markers
        todo_issues = self.find_todo_markers()
        issues.extend(todo_issues)
        
        # Check for potential memory leaks
        memory_issues = self.check_memory_issues()
        issues.extend(memory_issues)
        
        # Check for hardcoded values
        hardcode_issues = self.check_hardcoded_values()
        issues.extend(hardcode_issues)
        
        return issues
    
    def find_todo_markers(self):
        """Find TODO, FIXME, XXX markers in code"""
        issues = []
        patterns = [r'TODO', r'FIXME', r'XXX', r'HACK']
        
        for pattern in ['**/*.h', '**/*.cpp']:
            for file_path in self.repo_path.glob(pattern):
                if 'build' in str(file_path) or '.git' in str(file_path):
                    continue
                    
                try:
                    with open(file_path, 'r') as f:
                        lines = f.readlines()
                        
                    for line_num, line in enumerate(lines, 1):
                        for marker in patterns:
                            if marker in line:
                                issues.append({
                                    "type": "todo_marker",
                                    "severity": "info",
                                    "file": str(file_path.relative_to(self.repo_path)),
                                    "line": line_num,
                                    "message": f"{marker} found: {line.strip()}",
                                    "detected_at": datetime.now().isoformat()
                                })
                except Exception:
                    continue
        
        return issues
    
    def check_memory_issues(self):
        """Check for potential memory issues"""
        issues = []
        
        # Check for unbounded containers
        market_h = self.repo_path / "include/market/market.h"
        if market_h.exists():
            with open(market_h, 'r') as f:
                content = f.read()
            
            if "push_back" in content and "size()" not in content and "clear()" in content:
                issues.append({
                    "type": "memory_issue",
                    "severity": "warning",
                    "file": "include/market/market.h",
                    "message": "Unbounded vector growth detected in Market class",
                    "detected_at": datetime.now().isoformat()
                })
        
        return issues
    
    def check_hardcoded_values(self):
        """Check for hardcoded values that should be configurable"""
        issues = []
        
        main_cpp = self.repo_path / "src/main.cpp"
        if main_cpp.exists():
            with open(main_cpp, 'r') as f:
                content = f.read()
            
            # Check for hardcoded product lists
            if '{"小麦", "パン", "道具"}' in content:
                issues.append({
                    "type": "hardcoded_value",
                    "severity": "info",
                    "file": "src/main.cpp",
                    "message": "Hardcoded product list found - consider making configurable",
                    "detected_at": datetime.now().isoformat()
                })
        
        return issues
    
    def check_integration_status(self):
        """Check which implemented features are not integrated"""
        issues = []
        
        main_cpp = self.repo_path / "src/main.cpp"
        if not main_cpp.exists():
            return issues
            
        with open(main_cpp, 'r') as f:
            main_content = f.read()
        
        # Check for unused features
        features_to_check = [
            ("TradeRoute", "system/trade_route.h"),
            ("Government", "agent/government.h"),
            ("LoanProvider", "agent/loan_provider.h")
        ]
        
        for feature_name, header_path in features_to_check:
            header_full_path = self.repo_path / "include" / header_path
            if header_full_path.exists():
                # Check if included but not used
                include_line = f'#include "{header_path}"'
                if include_line in main_content and feature_name not in main_content:
                    issues.append({
                        "type": "integration_gap",
                        "severity": "enhancement",
                        "file": "src/main.cpp",
                        "message": f"{feature_name} is included but not used in main simulation",
                        "detected_at": datetime.now().isoformat()
                    })
        
        return issues
    
    def run_full_scan(self):
        """Run comprehensive issue detection"""
        print("🔍 Running comprehensive issue scan...")
        
        all_issues = []
        
        # Build analysis
        print("  📦 Analyzing build warnings...")
        build_issues = self.run_build_analysis()
        all_issues.extend(build_issues)
        
        # Code quality
        print("  📝 Checking code quality...")
        quality_issues = self.check_code_quality_issues()
        all_issues.extend(quality_issues)
        
        # Integration status
        print("  🔗 Checking integration status...")
        integration_issues = self.check_integration_status()
        all_issues.extend(integration_issues)
        
        # Create scan record
        scan_record = {
            "timestamp": datetime.now().isoformat(),
            "total_issues": len(all_issues),
            "severity_breakdown": self.categorize_issues(all_issues),
            "issues": all_issues
        }
        
        # Load and update history
        history = self.load_history()
        history["scans"].append(scan_record)
        
        # Keep only last 10 scans
        if len(history["scans"]) > 10:
            history["scans"] = history["scans"][-10:]
        
        self.save_history(history)
        
        return scan_record
    
    def categorize_issues(self, issues):
        """Categorize issues by severity and type"""
        breakdown = {
            "by_severity": {},
            "by_type": {}
        }
        
        for issue in issues:
            severity = issue.get("severity", "unknown")
            issue_type = issue.get("type", "unknown")
            
            breakdown["by_severity"][severity] = breakdown["by_severity"].get(severity, 0) + 1
            breakdown["by_type"][issue_type] = breakdown["by_type"].get(issue_type, 0) + 1
        
        return breakdown
    
    def generate_report(self, scan_record):
        """Generate human-readable report"""
        report = f"""# Issue Detection Report
Generated: {scan_record['timestamp']}

## Summary
- **Total Issues**: {scan_record['total_issues']}

### By Severity
"""
        
        for severity, count in scan_record['severity_breakdown']['by_severity'].items():
            emoji = {"error": "🔴", "warning": "🟡", "info": "ℹ️", "enhancement": "🔧"}.get(severity, "❓")
            report += f"- {emoji} **{severity.title()}**: {count}\n"
        
        report += "\n### By Type\n"
        for issue_type, count in scan_record['severity_breakdown']['by_type'].items():
            report += f"- **{issue_type.replace('_', ' ').title()}**: {count}\n"
        
        report += "\n## Detailed Issues\n"
        for i, issue in enumerate(scan_record['issues'], 1):
            severity_emoji = {"error": "🔴", "warning": "🟡", "info": "ℹ️", "enhancement": "🔧"}.get(issue.get('severity'), "❓")
            report += f"\n### {i}. {severity_emoji} {issue.get('message', 'No message')}\n"
            if 'file' in issue:
                report += f"**File**: `{issue['file']}`"
                if 'line' in issue:
                    report += f":{issue['line']}"
                report += "\n"
        
        return report

if __name__ == "__main__":
    tracker = ContinuousIssueTracker("/home/runner/work/MiddleAgeEconomySim/MiddleAgeEconomySim")
    scan_result = tracker.run_full_scan()
    
    print(f"\n📊 Scan Results:")
    print(f"Total issues: {scan_result['total_issues']}")
    
    print(f"\nBy severity:")
    for severity, count in scan_result['severity_breakdown']['by_severity'].items():
        print(f"  {severity}: {count}")
    
    print(f"\nBy type:")
    for issue_type, count in scan_result['severity_breakdown']['by_type'].items():
        print(f"  {issue_type}: {count}")
    
    # Generate and save report
    report = tracker.generate_report(scan_result)
    report_path = Path("/tmp/latest_issue_report.md")
    with open(report_path, 'w') as f:
        f.write(report)
    
    print(f"\n📄 Detailed report saved to: {report_path}")