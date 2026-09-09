Code Architecture and Engineering Rules

1 Codebase harus memiliki struktur yang jelas

Jangan membuat struktur folder hanya berdasarkan asumsi atau kebiasaan

Setiap folder dan file harus memiliki alasan keberadaan yang jelas

Jangan membuat folder hanya untuk menampung satu file jika tidak memberikan boundary yang berarti

Jangan memasukkan seluruh logic ke dalam satu file

Jangan membuat terlalu banyak file kecil hanya untuk memecah code tanpa alasan


2 Gunakan Deep Module

Prefer deep modules over shallow modules

Sebuah module harus memiliki interface yang sederhana dengan implementation yang cukup kuat di dalamnya

Jangan memaksa caller memahami detail internal sebuah module

Hide implementation details whenever possible

Expose only what other parts of the system actually need


3 Feature Boundary

Organize code around meaningful features and responsibilities

Feature yang berbeda harus memiliki boundary yang jelas

Jangan mencampur unrelated functionality dalam satu file

Jangan membuat satu shared folder sebagai tempat membuang semua code yang tidak tahu harus diletakkan di mana

Shared code hanya boleh dibuat ketika memang digunakan oleh beberapa bagian sistem dan memiliki abstraction yang stabil


4 One File One Responsibility

Satu file harus memiliki satu responsibility utama

Satu file dapat memiliki beberapa method atau class jika semuanya berada dalam satu cohesive responsibility

Jangan membuat file yang menjadi tempat berbagai unrelated functionality

Jika sebuah file terus membesar dan mulai memiliki beberapa responsibility yang berbeda evaluasi kembali boundary module tersebut


5 One Feature One Identifier

Setiap feature harus memiliki identifier yang jelas jika sistem memang membutuhkan identifier

Identifier harus konsisten digunakan pada
Database
API
UI
Logging
Analytics
Navigation
State management
File reference

Jangan membuat identifier berbeda untuk object yang sebenarnya merepresentasikan entity yang sama tanpa alasan


6 Folder Structure

Folder structure harus mencerminkan architecture dan dependency boundaries

Contoh struktur konseptual

src
features
feature_a
feature_b

core
network
database
storage
logging
config

shared
ui
components
utils

docs
adr
README

Jangan mengikuti struktur tersebut secara literal jika architecture project membutuhkan struktur berbeda

Structure harus mengikuti kebutuhan project


7 Avoid God Files

Jangan membuat file yang mengandung terlalu banyak responsibility

Contoh yang harus dihindari

API request
Database access
Business logic
UI state
Validation
Formatting
Navigation
Configuration

semuanya berada dalam satu file


8 Avoid God Classes

Class harus memiliki responsibility yang jelas

Jika sebuah class mulai menangani terlalu banyak concern evaluasi boundary dan pecah berdasarkan responsibility

Jangan memecah class hanya berdasarkan jumlah baris

Cohesion lebih penting daripada line count


9 Avoid Trash Code

Jangan mempertahankan code yang tidak memiliki fungsi

Hapus
Dead code
Unused variable
Unused method
Unused import
Duplicate logic
Unreachable code
Temporary workaround yang sudah tidak diperlukan
Debug code
Console spam
Commented out code

Jangan meninggalkan code dengan alasan mungkin nanti dibutuhkan


10 No Duplicate Logic

Jika logic yang sama muncul di beberapa tempat evaluasi apakah logic tersebut benar benar perlu menjadi shared abstraction

Jangan membuat abstraction terlalu dini

Duplication harus dihilangkan ketika duplication tersebut memiliki behavior yang sama dan abstraction yang dihasilkan tetap jelas


11 Naming

Nama variable method class file module dan constant harus menjelaskan intent

Gunakan nama yang spesifik

Hindari nama seperti

data
temp
thing
stuff
manager
helper
util
misc
test2
newData
finalData
result2

kecuali memang memiliki konteks yang jelas


12 Boolean Naming

Boolean harus memiliki nama yang jelas menunjukkan state

Prefer

isEnabled
isLoading
hasPermission
canRetry
shouldRefresh

daripada

enabled
loading
permission
retry
refresh

kecuali context sudah membuatnya jelas


13 Method Naming

Method harus menjelaskan action yang dilakukan

Prefer

loadUserProfile
saveSettings
validateInput
calculateTotal

daripada

process
handle
doSomething
run
execute

Jika method bernama generic pastikan context dan responsibility method tersebut memang jelas


14 Method Size

Jangan memecah method secara mekanis hanya agar terlihat pendek

Extract method ketika terdapat logical boundary yang jelas

Method harus mudah dibaca dari atas ke bawah tanpa membutuhkan mental reconstruction yang berlebihan


15 Control Flow

Prefer simple control flow

Reduce unnecessary nesting

Use early return when it makes the logic easier to follow

Avoid deeply nested if else structures

Jangan menggunakan clever syntax jika syntax tersebut membuat code lebih sulit dipahami


16 Syntax

Gunakan syntax yang idiomatic untuk language dan framework yang digunakan

Jangan menggunakan syntax hanya karena terlihat lebih advanced

Readability dan correctness lebih penting daripada menunjukkan kemampuan menggunakan language feature tertentu


17 Type Safety

Gunakan type system secara maksimal jika language mendukungnya

Hindari unnecessary casting

Hindari unsafe conversion

Hindari penggunaan generic type ketika type yang lebih spesifik tersedia

Jangan menyembunyikan type error dengan workaround


18 Nullability

Handle null dan missing value secara explicit

Jangan menggunakan null check secara acak

Tentukan contract dengan jelas

Jika sebuah value tidak boleh null maka enforce invariant tersebut

Jika value memang optional maka representasikan sebagai optional state yang jelas


19 Error Handling

Error harus ditangani pada boundary yang tepat

Jangan catch exception hanya untuk membuat error hilang

Jangan menggunakan empty catch block

Jangan menelan error tanpa logging atau recovery yang sesuai

Error harus memiliki recovery strategy jika recovery memungkinkan


20 Error Messages

Error message harus menjelaskan masalah dan context yang relevan

Jangan menggunakan generic message jika informasi yang lebih berguna dapat diberikan

User facing error dan developer facing error harus dipisahkan

Jangan membocorkan
Token
Password
API key
Internal path
Database query
Stack trace
Sensitive information


21 Logging

Logging harus membantu debugging dan maintenance

Log hanya informasi yang relevan

Jangan melakukan logging berlebihan pada hot path

Jangan log sensitive information

Gunakan level logging yang sesuai

Debug information tidak boleh menjadi bagian dari normal production output


22 Comments

Komentar harus menjelaskan WHY bukan mengulang WHAT

Jangan menulis komentar yang hanya menerjemahkan syntax

Bad

Increment i by one

Good

Skip the first entry because the API reserves index zero for metadata

Komentar harus tetap benar ketika implementation berubah


23 Comments Must Not Lie

Jika code berubah dan komentar menjadi salah komentar tersebut harus diperbaiki atau dihapus

Outdated comments lebih buruk daripada tidak ada comment

Jangan menulis dokumentasi yang menjelaskan behavior yang sebenarnya tidak dilakukan code


24 Comment Style

Komentar harus singkat langsung dan ditulis untuk developer yang akan maintain code tersebut

Gunakan bahasa Inggris

Gunakan tone profesional dan technical

Jangan menggunakan komentar seperti

This is important
Do not touch this
Magic happens here
Trust me
Very complicated
AI generated
Standard industry practice

Komentar harus menjelaskan technical reasoning ketika reasoning tersebut tidak obvious dari code


25 Avoid Commented Out Code

Jangan meninggalkan implementation lama dalam bentuk commented out code

Gunakan version control untuk historical code

Source file harus berisi code yang benar benar digunakan


26 TODO

TODO hanya boleh digunakan jika memang terdapat pekerjaan yang belum selesai

TODO harus menjelaskan action yang diperlukan

Hindari TODO tanpa context

Bad

TODO fix this

Good

TODO Replace polling with event driven updates once the backend exposes change notifications


27 Documentation

Documentation harus menjelaskan architecture dan decision boundary bukan menduplikasi source code

Jangan membuat documentation yang hanya menjelaskan ulang apa yang sudah jelas dari code

Jika developer harus membaca documentation lalu membaca seluruh code untuk mengetahui behavior sebenarnya berarti structure kemungkinan terlalu lemah


28 Documentation Locality

Keep documentation close to the decision or architecture it describes

Information yang hanya relevan terhadap satu feature sebaiknya berada dekat dengan feature tersebut

Information yang bersifat architectural dapat ditempatkan pada architecture documentation atau ADR

Jangan membuat satu dokumentasi besar yang menjadi dumping ground


29 README

README harus ringkas tetapi cukup untuk membuat developer memahami project dengan cepat

README minimal menjelaskan

Project overview
Core features
Architecture overview
Folder structure
Requirements
Setup
Configuration
Build
Run
Test
Deployment
Environment variables
Known limitations
Troubleshooting

Jangan menjelaskan setiap method atau class di README


30 README Must Stay Accurate

README harus diperbarui ketika perubahan architecture atau setup membuat informasi sebelumnya tidak lagi benar

Jangan menambahkan dokumentasi hanya karena terlihat lengkap

Documentation harus mencerminkan actual codebase


31 Architecture Decision Records

Gunakan ADR untuk architectural decisions yang memiliki trade off atau konsekuensi jangka panjang

ADR harus menjelaskan

Context
Decision
Alternatives considered
Consequences

Gunakan ADR untuk keputusan seperti

Why a database was selected
Why a specific architecture was selected
Why a dependency was rejected
Why a communication pattern was selected
Why a performance trade off was accepted


32 Do Not Use Documentation as Source of Truth for Implementation Details

Source code adalah source of truth untuk behavior implementation

Documentation harus menjelaskan intent architecture constraints dan decisions

Jangan membuat documentation yang mencoba menggantikan source code


33 Documentation Duplication

Avoid documenting the same fact in multiple places

Jika satu perubahan harus memperbarui banyak dokumentasi yang identik kemungkinan terdapat duplication yang tidak perlu

Prefer one authoritative source


34 API Contracts

API contract harus jelas

Document

Request
Response
Authentication
Error behavior
Required fields
Optional fields
Constraints

Jangan membuat documentation yang berbeda dari actual API behavior


35 Configuration

Configuration harus dipisahkan dari business logic

Jangan hardcode

API key
Secret
Environment specific URL
Credential
Deployment specific value

Gunakan configuration mechanism yang sesuai dengan platform


36 Secrets

Never commit secrets into source code

Never place credentials in README

Never expose secrets in logs

Never expose secrets in client side code when they are intended to remain server side


37 Dependencies

Jangan menambahkan dependency hanya untuk menyelesaikan masalah kecil yang dapat ditangani dengan existing platform APIs

Sebelum menambahkan dependency evaluasi

Maintenance cost
Bundle size
Security
Compatibility
Performance
License
Actual necessity

Remove dependencies that are no longer required


38 Stable Architecture

Architecture harus dipilih berdasarkan actual requirements

Jangan menggunakan architecture pattern hanya karena populer

Jangan menambahkan abstraction layer yang belum memiliki kebutuhan nyata


39 Avoid Premature Abstraction

Do not abstract code merely because it looks similar

Abstract when the behavior and responsibility are actually stable

Prefer simple code over speculative architecture


40 Performance

Optimasi berdasarkan actual bottleneck

Jangan melakukan micro optimization tanpa alasan

Prioritaskan

Algorithmic complexity
Memory usage
I O
Network usage
Database access
Rendering cost
Startup time
Hot paths

Jangan mengorbankan readability untuk optimization yang tidak memberikan measurable benefit


41 Replace Unstable Code

Jika menemukan implementation yang fragile unstable atau memiliki failure mode yang jelas

Jangan hanya menambahkan workaround di atas implementation tersebut

Evaluasi root cause

Kemudian replace implementation jika diperlukan


42 Root Cause First

Fix root cause instead of repeatedly patching symptoms

Jika sebuah bug membutuhkan banyak conditional workaround evaluasi kembali design atau abstraction yang menyebabkan masalah tersebut


43 Concurrency

Concurrency harus explicit

Jangan membuat race condition melalui shared mutable state

Thread safety harus dipertimbangkan ketika data diakses dari beberapa execution context


44 Resource Management

Resource harus memiliki lifecycle yang jelas

Pastikan

File handle ditutup
Database connection dikelola
Network request dapat dibatalkan jika relevan
Listener dilepas
Observer lifecycle safe
Background task tidak terus berjalan setelah owner dihancurkan


45 Memory

Hindari

Memory leak
Unbounded cache
Unnecessary object allocation
Large object retention
Repeated expensive allocation

Gunakan lifecycle dan ownership yang jelas


46 UI Code

UI layer harus fokus pada presentation dan user interaction

Business logic tidak boleh tersebar di UI component

Database dan network access tidak boleh dilakukan secara sembarangan langsung dari UI layer


47 Business Logic

Business rules harus memiliki tempat yang jelas

Jangan menyebarkan business rule ke

UI
Adapter
Controller
Utility
Database callback

secara acak


48 Data Layer

Data access harus memiliki boundary yang jelas

UI tidak seharusnya mengetahui detail implementasi database atau network

Jika storage implementation berubah bagian lain dari aplikasi seharusnya tidak perlu mengetahui detail tersebut


49 Testing

Test harus mengikuti behavior dan responsibility

Prioritaskan test untuk

Business logic
Critical flows
Parsing
Validation
Data transformation
Error handling
Security sensitive behavior

Jangan membuat test hanya untuk meningkatkan jumlah coverage


50 Test Naming

Test name harus menjelaskan behavior yang diuji

Nama test harus menjawab

Given
When
Then

atau menjelaskan kondisi dan expected result secara langsung


51 No Fake Tests

Jangan membuat test yang hanya menjalankan code tanpa assertion yang berarti

Jangan mock semuanya sampai test tidak lagi merepresentasikan behavior nyata


52 Refactoring

Refactor secara incremental

Jangan mengubah architecture besar besaran tanpa memahami dependency dan behavior yang sudah ada

Sebelum refactor pahami

Entry points
Dependencies
Data flow
State flow
Side effects
Error paths


53 Before Editing Existing Code

Sebelum mengubah code

Read relevant files
Trace dependencies
Understand data flow
Identify callers
Identify side effects
Check tests
Check configuration
Check related documentation

Jangan melakukan rewrite berdasarkan satu file yang terlihat bermasalah


54 Locality

Keep related code close together

Developer harus dapat memahami sebuah feature dengan membaca area code yang terbatas

Jika memahami satu feature membutuhkan berpindah ke terlalu banyak file tanpa alasan yang jelas evaluasi structure tersebut


55 Change Locality

Perubahan kecil seharusnya tidak membutuhkan perubahan pada banyak module yang tidak berhubungan

Minimize unnecessary coupling

Prefer architecture yang membuat feature dapat berubah dengan impact area yang kecil


56 Dependency Direction

Dependency harus memiliki arah yang jelas

Higher level policy tidak boleh bergantung secara langsung pada low level implementation detail jika abstraction yang stabil memang diperlukan

Jangan membuat circular dependency


57 Public API

Setiap module harus memiliki public surface yang minimal

Jangan expose internal implementation hanya karena lebih mudah

Public API harus stabil dan sengaja dirancang


58 Internal Implementation

Detail internal harus tetap internal

Jika sesuatu tidak diperlukan oleh caller jangan expose

Smaller public API means fewer contracts to maintain


59 Naming Consistency

Gunakan terminology yang sama di seluruh project

Jika entity disebut User jangan gunakan

Account
Profile
Person
Member

secara bergantian jika semuanya merujuk pada concept yang sama


60 No Random Formatting

Formatting harus konsisten

Gunakan formatter dan linter yang sesuai dengan language

Jangan melakukan formatting berbeda beda antar file


61 No Random Comments

Komentar tidak boleh digunakan untuk membuat file terlihat lebih lengkap

Jika code sudah jelas jangan tambahkan komentar yang tidak memberikan informasi baru


62 No Decorative Comments

Jangan menggunakan komentar sebagai dekorasi visual

Jangan menggunakan

======
------
########
********
>>>>>>>>


63 Professional Developer Communication

Komentar dan technical documentation ditulis dalam bahasa Inggris

Gunakan bahasa yang direct dan technical

Write for maintainers

Explain intent
Explain constraints
Explain trade offs
Explain non obvious behavior

Do not write comments as if explaining code to a beginner when the code is already self explanatory


64 Code Review Mindset

Setiap perubahan harus dapat dijelaskan

Why was this changed
What behavior changes
What dependencies are affected
What failure modes are introduced
What performance impact exists
What assumptions are made

Jika perubahan tidak dapat dijelaskan dengan jelas evaluasi kembali implementation


65 No Cargo Cult Code

Jangan menyalin pattern atau architecture hanya karena project lain menggunakannya

Setiap abstraction harus memiliki alasan yang dapat dijelaskan


66 No Magic Numbers

Nilai constant yang memiliki semantic meaning harus memiliki nama

Jangan menggunakan angka atau string literal berulang jika nilai tersebut merupakan bagian dari domain rule atau configuration


67 No Hidden Side Effects

Method yang terlihat seperti operasi sederhana tidak boleh diam diam melakukan unrelated side effects

Jika method memiliki side effect penting pastikan contract dan naming membuatnya jelas


68 Predictable Behavior

Code harus memiliki behavior yang dapat diprediksi

Avoid surprising mutation

Avoid implicit global state

Avoid hidden initialization

Avoid order dependent behavior


69 Backward Compatibility

Ketika mengubah public API atau persisted data

Evaluate existing callers
Stored data
Migration requirements
Compatibility
Rollback strategy

Jangan mengubah contract tanpa memeriksa impact


70 Database Changes

Database schema changes harus memiliki migration strategy jika database tersebut persistent

Jangan mengubah schema secara langsung tanpa mempertimbangkan existing data


71 Data Validation

Validate data pada boundary

Jangan mengasumsikan external input selalu valid

External input termasuk

User input
Network response
File
Database
Intent
Environment variable
Configuration


72 Security

Security harus menjadi bagian dari architecture bukan patch setelah implementation selesai

Consider

Input validation
Authentication
Authorization
Secret management
Data exposure
Storage
Network communication
Logging


73 Privacy

Minimize collected data

Jangan mengumpulkan data yang tidak diperlukan

Jangan menyimpan sensitive data tanpa alasan yang jelas

Jangan mengirim data ke external service tanpa requirement yang jelas


74 Build Quality

Code harus buildable setelah perubahan

Jangan meninggalkan

Compilation error
Broken import
Missing dependency
Invalid configuration
Unused required resource
Dead reference


75 Lint and Static Analysis

Jika project memiliki formatter linter atau static analyzer gunakan tools tersebut

Jangan disable warning hanya untuk membuat build terlihat bersih

Jika warning memang false positive gunakan suppression secara lokal dengan alasan yang jelas


76 Performance Regression

Setiap perubahan yang menyentuh hot path harus mempertimbangkan performance impact

Jangan mengganti implementation stabil dengan implementation yang lebih kompleks tanpa benefit yang jelas


77 Failure Modes

Setiap feature harus dianalisis terhadap failure mode

Consider

Invalid input
Missing data
Network failure
Timeout
Permission denied
Authentication failure
Storage failure
Concurrency issue
Resource exhaustion
Unexpected external response


78 Graceful Failure

Ketika failure tidak dapat dihindari

Fail predictably
Preserve user data when possible
Provide recovery
Keep unaffected functionality working

Jangan membuat satu failure menjatuhkan seluruh application jika isolation memungkinkan


79 Remove Technical Debt During Refactoring

Jika refactor menemukan code yang jelas obsolete dan aman untuk dihapus

Remove it

Jangan membuat architecture baru di atas technical debt yang sebenarnya sudah tidak diperlukan


80 Keep Changes Focused

Satu perubahan harus memiliki scope yang jelas

Jangan mencampur

Feature implementation
Unrelated refactor
Formatting seluruh project
Dependency migration
Architecture rewrite

dalam satu perubahan tanpa alasan


81 README Structure

README harus menggunakan struktur yang mudah dipindai

Project Name

Overview

Features

Architecture

Project Structure

Requirements

Installation

Configuration

Usage

Development

Testing

Build

Deployment

Troubleshooting

Known Limitations

Architecture Decisions


82 Architecture Documentation

Architecture documentation harus menjelaskan

System boundaries
Major components
Data flow
Dependency direction
External services
Persistence
Important constraints

Jangan menjelaskan setiap implementation detail


83 ADR Structure

Setiap ADR harus memiliki

Title
Status
Context
Decision
Alternatives
Consequences

ADR harus menjelaskan decision bukan menjadi tutorial implementation


84 Source Code Is the Implementation Authority

Jika documentation bertentangan dengan actual behavior

Treat the source code as the implementation authority

Kemudian update documentation atau ADR agar kembali sesuai dengan actual decision


85 AI Agent Must Inspect Before Refactoring

Before refactoring existing code

Inspect the repository structure
Inspect relevant files
Trace dependencies
Trace data flow
Trace state flow
Inspect configuration
Inspect tests
Inspect build configuration
Inspect related documentation
Inspect ADRs when available

Do not refactor from a partial context


86 AI Agent Must Preserve Behavior

Refactoring harus mempertahankan behavior existing kecuali perubahan behavior memang merupakan tujuan task

Jika behavior berubah secara sengaja jelaskan perubahan tersebut


87 AI Agent Must Prefer Deep Modules

When restructuring code prefer modules with

Small clear interfaces
Strong internal implementation
High cohesion
Low coupling
Clear ownership

Do not create shallow wrappers that only forward calls without adding meaningful abstraction


88 AI Agent Must Remove Trash Code

During refactoring identify and remove when safe

Dead code
Duplicate code
Unused imports
Unused dependencies
Unused variables
Redundant abstraction
Obsolete comments
Debug statements
Temporary hacks
Unreachable branches

Do not remove code blindly if its usage is unclear


89 AI Agent Must Optimize Carefully

Do not optimize based on intuition alone

Identify the actual bottleneck

Then optimize the relevant

Algorithm
Memory
I O
Network
Database
Rendering
Startup
Concurrency

Prefer simple optimizations with clear impact


90 AI Agent Must Not Create Documentation Redundancy

Do not create documentation that merely repeats the implementation

If a design decision matters create an ADR

If usage matters document usage

If architecture matters document architecture

If implementation is obvious from code do not document it again


91 AI Agent Final Verification

Before considering the implementation complete verify

Project structure is coherent
File responsibilities are clear
Naming is consistent
Dependencies have clear direction
No unnecessary abstraction exists
No dead code remains
No duplicate logic remains
No fake functionality exists
No unstable workaround remains without justification
Error handling is complete
Resource lifecycle is safe
Performance is acceptable
Security requirements are respected
Documentation matches actual behavior
README is updated when necessary
ADR exists for significant architectural decisions
Comments explain intent rather than syntax
Code formatting is consistent
Lint passes
Tests pass
Build succeeds


92 Final Principle

Write code so another maintainer can understand the system by reading a small and relevant portion of the codebase

Keep related decisions close together

Keep interfaces small

Keep responsibilities clear

Keep dependencies explicit

Keep comments truthful

Keep documentation useful

Keep failure modes predictable

Keep the implementation simple unless complexity is justified